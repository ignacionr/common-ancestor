FROM nginx
RUN rm /etc/nginx/conf.d/default.conf
COPY ./balancer.conf /etc/nginx/conf.d/balancer.conf
EXPOSE 8080
