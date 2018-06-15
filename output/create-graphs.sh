#!/bin/bash
# Author Thomas Wood

set -x

DIR=/home/weather/data/
OUTDIR=/home/weather/public_html/

CSV=/home/weather/data/weather.txt

RRD=$OUTDIR/weather.rrd

if [ "$1" != 'nocsv' ]; then

  echo "Creating CSV"
  rm $DIR/log.bin
#  INPUT=`ls $DIR/2015*.bin`
#  cat $INPUT > $DIR/log.bin
  cat $DIR/log.bin.20150927 ~weather/weather-log.bin > $DIR/log.bin
  /home/weather/bin/csv

  echo "Sending any weather alerts"
  python ~/bin/weather-alert.py

  cp $DIR/current-reading.html $OUTDIR

  echo "Creating $RRD"

# each data point is 60 seconds apart
# 1440 minutes in 1 day
# 744 hours in 1 month (31 days)
# 264864 hours in 1 year

#time:temp0:hum0:dew0:temp1:hum1:dew1:temp2:hum2:dew2:wind-direction:gust:wind-average:wind-chill:pressure:rain-rate:rain-last-hour:rain-last-24hrs

# 1295392920 was time of the first reading

  rrdtool create $RRD --step 60 --start 1295392000 \
    DS:temp0:GAUGE:120:U:U \
    DS:hum0:GAUGE:120:0:100 \
    DS:dew0:GAUGE:120:U:U \
    DS:temp1:GAUGE:120:U:U \
    DS:hum1:GAUGE:120:0:100 \
    DS:dew1:GAUGE:120:U:U \
    DS:temp2:GAUGE:120:U:U \
    DS:hum2:GAUGE:120:0:100 \
    DS:dew2:GAUGE:120:U:U \
    DS:winddir:GAUGE:120:0:360 \
    DS:gust:GAUGE:120:0:U \
    DS:windavg:GAUGE:120:0:U \
    DS:windchill:GAUGE:120:0:U \
    DS:pressure:GAUGE:120:950:1050 \
    DS:rainrate:GAUGE:120:0:U \
    DS:rain1hr:GAUGE:120:0:U \
    DS:rain24hr:GAUGE:120:0:U \
    DS:uv:GAUGE:120:0:U \
    RRA:AVERAGE:0.5:1:1500 \
    RRA:AVERAGE:0.5:15:700 \
    RRA:AVERAGE:0.5:60:8760 \
    RRA:MAX:0.5:1:1500 \
    RRA:MAX:0.5:15:700 \
    RRA:MAX:0.5:60:8760

# 1 sample average for 1440 samples (every minute for 1 day)
# 15 sample average for 672 samples (every 15 minuets for 1 week)
# 60 sample average for 8760 samples (every hour for 1 year)

# 1 day min/max (1440 samples) for 356 days

  echo "Updating $RRD"

  sed -e '/#.*/d' $CSV | xargs rrdtool update $RRD;

fi

declare -a grid
grid[0]="--x-grid HOUR:1:DAY:1:HOUR:4:0:%R"
grid[1]="--x-grid HOUR:6:DAY:1:DAY:1:86400:%a"
grid[2]="--x-grid DAY:1:WEEK:1:DAY:7:86400:%d/%m"
grid[3]="--x-grid WEEK:1:MONTH:1:MONTH:1:2592000:%b"
counter=0

  for DURATION in day week month year day-large week-large month-large year-large; do

    echo "Creating $DURATION graph"

    SIZE='-w 320 -h 160'
    echo $DURATION | grep 'large'
    if [ $? == 0 ]; then
      SIZE='-w 1300 -h 500'
    fi

    START=`echo $DURATION| sed 's/-large//'`
    COMMON_OPTIONS="--slope-mode --start -1$START $SIZE --color MGRID#000 --color BACK#eee --border=0 "


    eval rrdtool graph $OUTDIR/temperature-$DURATION.png $COMMON_OPTIONS \
      DEF:temp0=$RRD:temp0:AVERAGE \
      DEF:temp1=$RRD:temp1:AVERAGE \
      DEF:temp2=$RRD:temp2:AVERAGE \
      DEF:dew=$RRD:dew1:AVERAGE \
      LINE2:temp2#f00:\"Temperature 2\\\l\" \
      LINE2:temp1#00f:\"Outdoor Temperature\\\l\" \
      LINE2:temp0#0f0:\"Indoor Temperature\\\l\" \
      LINE2:dew#0ff:\"Dew Point\\\l\" \
      LINE:0#000 \
      ${grid[$counter]} \
      --title "Temperature"


    eval rrdtool graph $OUTDIR/humidity-$DURATION.png $COMMON_OPTIONS \
      DEF:hum0=$RRD:hum0:AVERAGE \
      DEF:hum1=$RRD:hum1:AVERAGE \
      LINE2:hum1#00F:\"Outdoor Humidity\\\l\" \
      LINE2:hum0#0f0:\"Indoor Humidity\\\l\" \
      LINE:0#000 \
      ${grid[$counter]} \
      --title "Humidity"

    eval rrdtool graph $OUTDIR/pressure-$DURATION.png $COMMON_OPTIONS --lower-limit 959 --rigid --upper-limit 1059 -Y -A -X 1 \
      DEF:p=$RRD:pressure:AVERAGE \
      LINE2:p#00F:\"Pressure\\\l\" \
      ${grid[$counter]} \
      --title "Pressure"

   eval  rrdtool graph $OUTDIR/rain-$DURATION.png $COMMON_OPTIONS --lower-limit 0 --rigid \
      DEF:r1hr=$RRD:rain1hr:AVERAGE \
      AREA:r1hr#00FF00:\"Rain last hour\\\l\" \
      ${grid[$counter]} \
      --title "Rain"

   eval rrdtool graph $OUTDIR/wind-$DURATION.png $COMMON_OPTIONS --lower-limit 0.0 --rigid \
      DEF:wind=$RRD:windavg:AVERAGE \
      DEF:windgust=$RRD:gust:MAX \
      LINE2:wind#0000FF:\"Average wind speed\\\l\" \
      LINE2:windgust#00FF00:\"Max gust speed\\\l\" \
      ${grid[$counter]} \
      --title "Wind"

    eval rrdtool graph $OUTDIR/wind-direction-$DURATION.png $COMMON_OPTIONS -h 100 --lower-limit 0 --upper-limit 360 --rigid \
      LINE:360#f00:\"North\" \
      LINE:0#f00: \
      LINE:90#0f0:\"East\" \
      LINE:180#00f:\"South\" \
      LINE:270#f0f:\"West\" \
      DEF:direction=$RRD:winddir:AVERAGE \
      LINE2:direction#000:\"Average direction\\\l\" \
      ${grid[$counter]}

    eval rrdtool graph $OUTDIR/uv-$DURATION.png $COMMON_OPTIONS \
      DEF:p=$RRD:uv:AVERAGE \
      LINE2:p#00F:\"UV\\\l\" \
      --title "UV" \
      ${grid[$counter]}

    let "counter += 1"
  done

echo "Done."

