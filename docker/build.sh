cd "$(dirname "$0")/.."

docker build -t ghcr.io/donghee/rc-security-module:latest -f docker/Dockerfile .

