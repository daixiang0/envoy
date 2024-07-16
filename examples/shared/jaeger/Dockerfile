FROM jaegertracing/all-in-one@sha256:516a9ebb078224eb949ad6f58e970591a16c87511c7012ef342f9e2174186c29
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
