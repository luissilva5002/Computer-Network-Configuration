pop3		110/tcp		pop-3		# POP version 3
http		80/tcp		www		# WorldWideWeb HTTP
ftp		21/tcp
smtp		25/tcp		mail


tux3
sudo ifconfig if_e1 192.168.1.1 netmask 255.255.255.0 up
ifconfig if_e1

tux4
sudo ifconfig if_e1 192.168.1.2 netmask 255.255.255.0 up
ifconfig if_e1


On tuxY3:
IP Address: 192.168.1.1
MAC Address: You can find it by running ifconfig if_e1 (look for ether).

On tuxY4:
IP Address: 192.168.1.2
MAC Address: You can find it by running ifconfig if_e1 (look for ether).

from tux3 
ping 192.168.1.2
or
from tux4
ping 192.168.1.1
