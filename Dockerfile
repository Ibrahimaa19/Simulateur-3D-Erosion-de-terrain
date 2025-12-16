FROM ubuntu:latest
WORKDIR /app
COPY . .
RUN apt-get update
RUN apt install -y \
    libglew2.2 libglew-dev \
    libglfw3 libglfw3-dev \
    libgl1-mesa-dri \
    libx11-6 \
    libxrandr2 \
    libxinerama1 \
    libxcursor1 \
    libxi6 \
    mesa-utils

RUN rm -rf /var/lib/apt/lists/*

