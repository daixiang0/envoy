FROM jaegertracing/all-in-one@sha256:69b98b1dd2239a31eed2e52efb1a41fd54859ef41750c8e4dba0e4af76791a11
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
