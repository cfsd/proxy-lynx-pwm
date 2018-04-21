
docker build -t proxy-lynx-pwm-v0.0.1 -f Dockerfile.armhf .
docker save  proxy-lynx-pwm-v0.0.1 > proxy-lynx-pwm-v0.0.1.tar
