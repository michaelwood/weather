#!/usr/bin/env python2

# Quick hack for certain weather alerts

import os

datadir = os.getenv("DATADIR", "/home/weather/data/")

#csv
# time:temp0:hum0:dew0:temp1:hum1:dew1:wind-direction:gust:wind-average:wind-chill:pressure:rain-rate:rain-last-hour:rain-last-24hrs

import subprocess
import smtplib
import pprint
from email.mime.text import MIMEText


alert_email = 'test@example.com'

alldata = {}

def sendemail(to, text):
    text =  "%s \n\nSummary:\n\n%s" % (text,
            pprint.pformat(alldata))

    msg = MIMEText(text)

    me = 'weather@michaelwood.me.uk'

    msg['Subject'] = 'STA Weather alert'
    msg['From'] = me
    msg['To'] = to

    s = smtplib.SMTP('localhost')
    s.sendmail(me, to, msg.as_string())
    s.quit()

def main():
    key = [ 'time',
            'temp0',
            'hum0',
            'dew0',
            'temp1',
            'hum1',
            'dew1',
            'temp2',
            'hum2',
            'dew2',
            'wind-direction',
            'gust',
            'wind-average',
            'wind-chill',
            'pressure',
            'rain-rate',
            'rain-last-hour',
            'rain-last-24hrs',
            'something else',
            ]

    weather = subprocess.check_output("tail -n 1 %s%s" % (datadir,"weather.txt"), shell=True)
    weather = weather.split(":")

    for i, item in enumerate(weather):
        alldata[key[i]] = float(item)

    # Rules of a weather warn

    if alldata['temp2'] < 5.0:
        sendemail(alert_email, "Temp 2 is below 5C (%.2f)" %
                alldata['temp2'])

    if alldata['temp2'] > 35.0:
        sendemail(alert_email, "Temp 2 is above 35C (%.2f)" %
                  alldata['temp2'])

main()
