FROM gcc:latest
WORKDIR /usr/src/app
COPY . .
RUN make all
EXPOSE 8080
CMD [ "./server" ]