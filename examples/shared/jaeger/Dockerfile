FROM jaegertracing/all-in-one@sha256:6b61afa516ef7febdf761d77e35bea8dd10a5b24a378540be5b516af7056f767
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
