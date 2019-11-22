DATE=`date '+%Y-%m-%d %H:%M:%S'`

echo "Motor control service started at ${DATE}"

sudo /home/pi/web-server/server
