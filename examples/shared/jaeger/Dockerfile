FROM jaegertracing/all-in-one@sha256:199ccdc8ceba681141aa7eae645a6332b87ee7f96668cb159eea85fc8835818a
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
