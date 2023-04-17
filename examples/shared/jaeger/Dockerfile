FROM jaegertracing/all-in-one@sha256:d229df6c4603729cca9239ca4bc8ce5f8e812fa65115508d6c8a79ce2117bbb8
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
