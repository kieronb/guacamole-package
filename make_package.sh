echo 'Installing dependencies'
apt-get install -y ruby-dev gcc libvncserver-dev libpulse-dev

echo 'Compiling'
./configure --with-init-dir=/etc/init.d
make
make install

mkdir ./package_dir

fpm -s dir -t deb -n guacamole -v 0.9.7 -C ./package_dir -p guacamole_VERSION_ARCH.deb -d "libcairo2" -d "libcairo2-dev" -d "tomcat7" -d "libvncserver0" -d "tightvncserver" -d "libpng12-0" -d "libpng12-dev" -d "libossp-uuid-dev" --deb-init /etc/init.d/guacd
