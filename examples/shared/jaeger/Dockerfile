FROM jaegertracing/all-in-one@sha256:3df70fec3cd7347806290bcc175d8810c505a5cf9aa01b65c4311339fd9a43ac
HEALTHCHECK \
    --interval=1s \
    --timeout=1s \
    --start-period=1s \
    --retries=60 \
    CMD wget -q --header='Content-Type:application/json' -O - http://localhost:14269/health | grep "Server available"
