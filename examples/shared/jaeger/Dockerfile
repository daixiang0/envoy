FROM jaegertracing/all-in-one@sha256:1ba904d35f97ea79bd536f14a489e16de9052ff3ecb9b29d9a2ec6a3372e953b
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
