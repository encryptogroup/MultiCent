FROM archlinux:base-devel-20250601.0.358000

# Install dependencies from repositories
RUN pacman -Sy
RUN pacman -S --noconfirm cmake boost git nlohmann-json ntl python3 python-pip
RUN pip install matplotlib --break-system-packages


# emp-tool
WORKDIR /home
RUN git clone https://github.com/emp-toolkit/emp-tool.git
WORKDIR emp-tool
RUN git checkout 8052d95
RUN cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 .
RUN make -j8
RUN make install
