FROM jaegertracing/all-in-one@sha256:8cf8d6eb8fa67269abcf94b2120624f0fcee480f1f7b0d98fb3b558d55db7e39
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
