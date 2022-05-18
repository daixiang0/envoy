#include "contrib/dlb/source/connection_balancer_impl.h"

#include <bits/types/FILE.h>
#include <sys/eventfd.h>
#include <unistd.h>

#include <algorithm>
#include <cstdlib>
#include <memory>

#ifndef DLB_DISABLED
#include "dlb.h"
#endif

namespace Envoy {
namespace Extensions {
namespace Dlb {

Envoy::Network::ConnectionBalancerSharedPtr
DLBConnectionBalanceFactory::createConnectionBalancerFromProto(
    const Protobuf::Message&, Server::Configuration::FactoryContext& context) {
#ifdef DLB_DISABLED
  throw EnvoyException("X86_64 architecture is required for Dlb.");
#endif

  // dlb init
  dlb_resources_t rsrcs;
  if (dlb_open(1, &dlb) == -1) {
    ExceptionUtil::throwEnvoyException(fmt::format("dlb_open {}", std::strerror(errno)));
  }
  if (dlb_get_dev_capabilities(dlb, &cap)) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_get_dev_capabilities {}", std::strerror(errno)));
  }

  if (dlb_get_num_resources(dlb, &rsrcs)) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_get_num_resources {}", std::strerror(errno)));
  }

  ENVOY_LOG(debug,
            "dlb available resources: {}, domains: {}, LDB queues: {}, LDB ports: {}, SN slots: {} "
            "{}, ES entries: {}, Contig ES entries: {}, LDB credits: {}, Contig LDB cred: {}, LDB "
            "credit pls: {}",
            rsrcs.num_sched_domains, rsrcs.num_ldb_queues, rsrcs.num_ldb_ports,
            rsrcs.num_sn_slots[0], rsrcs.num_sn_slots[1], rsrcs.num_ldb_event_state_entries,
            rsrcs.max_contiguous_ldb_event_state_entries, rsrcs.num_ldb_credits,
            rsrcs.max_contiguous_ldb_credits, rsrcs.num_ldb_credit_pools);

  uint num_seq_numbers;
  if (dlb_get_ldb_sequence_number_allocation(dlb, 0, &num_seq_numbers)) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_get_ldb_sequence_number_allocation {}", std::strerror(errno)));
  }

  domain_id = createSchedDomain(dlb, rsrcs, cap);
  if (domain_id == -1) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_create_sched_domain_ {}", std::strerror(errno)));
  }

  domain = dlb_attach_sched_domain(dlb, domain_id);
  if (domain == nullptr) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_attach_sched_domain {}", std::strerror(errno)));
  }

  const int partial_resources = 100;
  if (!cap.combined_credits) {
    int max_ldb_credits = rsrcs.num_ldb_credits * partial_resources / 100;
    int max_dir_credits = rsrcs.num_dir_credits * partial_resources / 100;

    ldb_pool_id = dlb_create_ldb_credit_pool(domain, max_ldb_credits);

    if (ldb_pool_id == -1) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("dlb_create_ldb_credit_pool {}", std::strerror(errno)));
    }

    dir_pool_id = dlb_create_dir_credit_pool(domain, max_dir_credits);

    if (dir_pool_id == -1) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("dlb_create_dir_credit_pool {}", std::strerror(errno)));
    }
  } else {
    int max_credits = rsrcs.num_credits * partial_resources / 100;

    ldb_pool_id = dlb_create_credit_pool(domain, max_credits);

    if (ldb_pool_id == -1) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("dlb_create_credit_pool {}", std::strerror(errno)));
    }
  }

  tx_queue_id = createLdbQueue(domain);
  if (tx_queue_id == -1) {
    ExceptionUtil::throwEnvoyException(fmt::format("tx create_ldb_queue {}", std::strerror(errno)));
  }

  const int num = context.options().concurrency();

  for (int i = 0; i < num; i++) {
    int tx_port_id = createLdbPort(domain, cap, ldb_pool_id, dir_pool_id);
    if (tx_port_id == -1) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("tx dlb_create_ldb_port {}", std::strerror(errno)));
    }

    dlb_port_hdl_t tx_port = dlb_attach_ldb_port(domain, tx_port_id);
    if (tx_port == nullptr) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("tx dlb_attach_ldb_port {}", std::strerror(errno)));
    }
    tx_ports.push_back(tx_port);

    int rx_port_id = createLdbPort(domain, cap, ldb_pool_id, dir_pool_id);
    if (rx_port_id == -1) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("rx dlb_create_ldb_port {}", std::strerror(errno)));
    }

    dlb_port_hdl_t rx_port = dlb_attach_ldb_port(domain, rx_port_id);
    if (rx_port == nullptr) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("rx dlb_attach_ldb_port {}", std::strerror(errno)));
    }
    rx_ports.push_back(rx_port);

    if (dlb_link_queue(rx_port, tx_queue_id, 0) == -1) {
      ExceptionUtil::throwEnvoyException(fmt::format("dlb_link_queue {}", std::strerror(errno)));
    }

    int efd = eventfd(0, EFD_NONBLOCK);
    if (efd < 0) {
      ExceptionUtil::throwEnvoyException(fmt::format("dlb eventfd {}", std::strerror(errno)));
    }
    if (dlb_enable_cq_epoll(rx_port, true, efd)) {
      ExceptionUtil::throwEnvoyException(
          fmt::format("dlb_enable_cq_epoll {}", std::strerror(errno)));
    }
    efds.push_back(efd);
  }

  if (dlb_launch_domain_alert_thread(domain, nullptr, nullptr)) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_launch_domain_alert_thread {}", std::strerror(errno)));
  }

  if (dlb_start_sched_domain(domain)) {
    ExceptionUtil::throwEnvoyException(
        fmt::format("dlb_start_sched_domain {}", std::strerror(errno)));
  }

  DlbConnectionBalanceFactorySingleton::initialize(this);

  return std::make_shared<DlbConnectionBalancerImpl>();
}

DLBConnectionBalanceFactory::~DLBConnectionBalanceFactory() {
  for (dlb_port_hdl_t port : rx_ports) {
    if (dlb_disable_port(port)) {
      ENVOY_LOG(error, "dlb_disable_port {}", std::strerror(errno));
    }
    if (dlb_detach_port(port) == -1) {
      ENVOY_LOG(error, "dlb_detach_port {}", std::strerror(errno));
    }
  }
  for (dlb_port_hdl_t port : tx_ports) {
    if (dlb_disable_port(port)) {
      ENVOY_LOG(error, "dlb_disable_port {}", std::strerror(errno));
    }
    if (dlb_detach_port(port) == -1) {
      ENVOY_LOG(error, "dlb_detach_port {}", std::strerror(errno));
    }
  }
  if (dlb_detach_sched_domain(domain) == -1) {
    ENVOY_LOG(error, "dlb_detach_sched_domain {}", std::strerror(errno));
  }

  if (dlb_reset_sched_domain(dlb, domain_id) == -1) {
    ENVOY_LOG(error, "dlb_reset_sched_domain {}", std::strerror(errno));
  }

  if (dlb_close(dlb) == -1) {
    ENVOY_LOG(error, "dlb_close {}", std::strerror(errno));
  }

  for (int fd : efds) {
    if (close(fd) == -1) {
      ENVOY_LOG(error, "dlb close fd {}", std::strerror(errno));
    }
  }
}

void DlbBalancedConnectionHandlerImpl::setDlbEvent() {
  auto listener = dynamic_cast<Envoy::Server::ActiveTcpListener*>(&handler_);

  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);
  dlb_event_ = listener->dispatcher().createFileEvent(
      DlbConnectionBalanceFactorySingleton::get().efds[index_],
      [this](uint32_t events) -> void { onDlbEvents(events); }, Event::FileTriggerType::Level,
      Event::FileReadyType::Read);
  dlb_event_->setEnabled(Event::FileReadyType::Read);
}

void DlbBalancedConnectionHandlerImpl::post(Network::ConnectionSocketPtr&& socket) {
  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);

  // The pointer will be casted to unique_ptr in onDlbEvents(), no need to consider free.
  auto s = socket.release();

  dlb_event_t events[1];
  events[0].send.queue_id = DlbConnectionBalanceFactorySingleton::get().tx_queue_id;
  events[0].send.sched_type = SCHED_UNORDERED;
  events[0].adv_send.udata64 = reinterpret_cast<std::uintptr_t>(s);
  int ret = dlb_send(DlbConnectionBalanceFactorySingleton::get().tx_ports[index_], 1, &events[0]);
  if (ret != 1) {
    ENVOY_LOG(error, "{} dlb fail send {}", name_, std::strerror(errno));
  } else {
    ENVOY_LOG(debug, "{} dlb send fd {}", name_, s->ioHandle().fdDoNotUse());
  }
}

void DlbBalancedConnectionHandlerImpl::onDlbEvents(uint32_t flags) {
  ASSERT(flags & (Event::FileReadyType::Read));

  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);

  dlb_event_t dlb_events[32];
  int num_rx = dlb_recv(DlbConnectionBalanceFactorySingleton::get().rx_ports.at(index_), 32, false,
                        dlb_events);
  if (num_rx == 0) {
    ENVOY_LOG(debug, "{} dlb receive none, skip", name_);
    return;
  } else {
    ENVOY_LOG(debug, "{} get dlb event {}", name_, num_rx);
  }

  int ret = dlb_release(DlbConnectionBalanceFactorySingleton::get().rx_ports.at(index_), num_rx);
  if (ret != num_rx) {
    ENVOY_LOG(debug, "{} dlb release {}", name_, std::strerror(errno));
  }
  for (int i = 0; i < num_rx; i++) {
    if (dlb_events[i].recv.error) {
      ENVOY_LOG(error, "{} dlb receive {}", name_, std::strerror(errno));
      continue;
    }

    if (dlb_events[i].recv.udata64) {
      const uint64_t data = dlb_events[i].recv.udata64;
      auto socket = reinterpret_cast<Network::ConnectionSocket*>(data);

      ENVOY_LOG(debug, "{} dlb recv {}", name_, socket->ioHandle().fdDoNotUse());
      auto listener = dynamic_cast<Envoy::Server::ActiveTcpListener*>(&handler_);
      listener->onAcceptWorker(std::unique_ptr<Network::ConnectionSocket>(socket),
                               listener->config_->handOffRestoredDestinationConnections(), true);
      listener->incNumConnections();
    }
  }
}

void DlbConnectionBalancerImpl::registerHandler(
    Envoy::Network::BalancedConnectionHandler& handler) {

  auto listener = dynamic_cast<Envoy::Server::ActiveTcpListener*>(&handler);

  const std::string worker_name = listener->dispatcher().name();
  const int index =
      std::stoi(worker_name.substr(worker_name.find_first_of('_') + 1, worker_name.size()));

  auto dlb_handler =
      std::make_shared<DlbBalancedConnectionHandlerImpl>(handler, index, worker_name);
  dlb_handler->setDlbEvent();

  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);
  DlbConnectionBalanceFactorySingleton::get().dlb_handlers.push_back(dlb_handler);
}

void DlbConnectionBalancerImpl::unregisterHandler(
    Envoy::Network::BalancedConnectionHandler& handler) {
  auto listener = dynamic_cast<Envoy::Server::ActiveTcpListener*>(&handler);
  auto worker_name = listener->dispatcher().name();
  int index = std::stoi(worker_name.substr(worker_name.find_first_of('_') + 1, worker_name.size()));

  // Now DLB does not support change config when running, clean DLB related config in DlbBalancedConnectionHandlerImpl
  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);
  auto dlb_handlers = DlbConnectionBalanceFactorySingleton::get().dlb_handlers;
  dlb_handlers.erase(dlb_handlers.begin() + index);
}

Envoy::Network::BalancedConnectionHandler& DlbConnectionBalancerImpl::pickTargetHandler(
    Envoy::Network::BalancedConnectionHandler& current_handler) {
  auto listener = dynamic_cast<Envoy::Server::ActiveTcpListener*>(&current_handler);
  auto worker_name = listener->dispatcher().name();
  const int index =
      std::stoi(worker_name.substr(worker_name.find_first_of('_') + 1, worker_name.size()));
  absl::MutexLock lock(&DlbConnectionBalanceFactorySingleton::get().lock);
  return *DlbConnectionBalanceFactorySingleton::get().dlb_handlers[index];
}

} // namespace Dlb
} // namespace Extensions
} // namespace Envoy
