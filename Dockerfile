from debian
RUN  apt update
RUN  apt install liblttng-ust-dev
ADD configure configure
RUN ./configure --enable-trace-backends=dtrace,ftrace,log,simple,syslog,ust