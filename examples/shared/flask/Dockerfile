FROM python:3.11.0b5-slim-bullseye@sha256:f91bba582bf087a2cb22b40e494de3af2bdda0ddde178f9535b6c608a5959fd4

ADD requirements.txt /tmp/flask-requirements.txt
RUN pip3 install -qr /tmp/flask-requirements.txt
RUN mkdir /code

ENTRYPOINT ["python3", "/code/service.py"]
