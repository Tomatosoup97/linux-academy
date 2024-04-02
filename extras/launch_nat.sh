export IFEXT=wlp0s20f3
export IFUSB=enxeeddc7e6cefa

echo "Setting up NAT for $IFUSB and $IFEXT"

ifconfig $IFUSB 192.168.0.1
echo 1 > /proc/sys/net/ipv4/ip_forward
/sbin/iptables -t nat -A POSTROUTING -o $IFEXT -j MASQUERADE
/sbin/iptables -A FORWARD -i $IFEXT -o $IFUSB -m state --state RELATED,ESTABLISHED -j ACCEPT
/sbin/iptables -A FORWARD -i $IFUSB -o $IFEXT -j ACCEPT
