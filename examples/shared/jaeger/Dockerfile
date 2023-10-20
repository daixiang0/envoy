FROM jaegertracing/all-in-one@sha256:0c498955ba078c4cf52578af008248050137c90e0a6b20dcecb1d6795c4b3be9
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
