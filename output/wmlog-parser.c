/*
 Weather binary dump to csv
 Author: Thomas Wood
*/

/*  vim: set cin sw=2 sts=2:  */
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <signal.h>
#include <ctype.h>

#include <time.h>

#include <math.h>

#include <glib.h>

enum
{
  DATE = 0x60,
  TEMP = 0x42,
  WIND = 0x48,
  PRESSURE = 0x46,
  RAIN = 0x41,
  UV = 0x47
};

char *forecast_str_day[] =
{
  "Partly cloudy",
  "Rainy",
  "Cloudy",
  "Sunny",
  "- Unknown -",
  "Snowy"
};


char *forecast_str_night[] =
{
  "Partly cloudy",
  "Rainy",
  "Cloudy",
  "Clear",
  "- Unknown -",
  "Snowy"
};

char *forecast_img_day[] =
{
  "<img id='forecast' src='img/weather-few-clouds.png'>",
  "<img id='forecast' src='img/weather-showers.png'>",
  "<img id='forecast' src='img/weather-overcast.png'>",
  "<img id='forecast' src='img/weather-clear.png'>",
  "- Unknown -",
  "<img id='forecast' src='img/weather-snow.png'>"
};


char *forecast_img_night[] =
{
  "<img id='forecast' src='img/weather-few-clouds-night.png'>",
  "<img id='forecast' src='img/weather-showers.png'>",
  "<img id='forecast' src='img/weather-overcast.png'>",
  "<img id='forecast' src='img/weather-clear-night.png'>",
  "- Unknown -",
  "<img id='forecast' src='img/weather-snow.png'>"
};

struct _WeatherData
{
  /* date and time */
  struct tm date;
  /* temp/humidity */
  float temp[4];
  float humidity[4];
  float dew_point[4];
  int   humidity_trend[4];
  int   temp_trend[4];

  /* wind */
  float wind_direction;
  float wind_gust;
  float wind_average;
  float wind_chill;

  /* pressure */
  float absolute_pressure;
  float relative_pressure;
  int forecast;

  /* rain */
  float rain_rate;
  float rain_last_hour;
  float rain_last_24hours;
  float total_rain;
  struct tm rain_reset_date;

  /* UV */
  int uv_index;
};

struct _WeatherData w_data;

static char *rose[17] = {
  "N",
  "NNE",
  "NE",
  "ENE",
  "E",
  "ESE",
  "SE",
  "SSE",
  "S",
  "SSW",
  "SW",
  "WSW",
  "W",
  "WNW",
  "NW",
  "NNW",
  "N"
};

static char *arrows[9] = {

  "↓",
  "↙",
  "←",
  "↖",
  "↑",
  "↗",
  "→",
  "↘",
  "↓",

};


static char *trend[3] = {
  "→",
  "↗",
  "↘"
};

char*
wind_str (float d)
{
  float n;
  char *str;

  /* normalise degrees into range 0..16 */

  n = d / 22.5;

  n = roundf (n);

  str = rose[(int) n];


  /* normalise between 0..8 for arrows */
  n = roundf (d / 45.0);
  str = g_strconcat (str, " ", arrows[(int) n], NULL);

  return str;
}

void print_record (FILE *outfile)
{
  static int time = 0;

  /* sanity check the date */
  if (w_data.date.tm_mday > 31) return;
  if (w_data.date.tm_mon > 12) return;

  if (mktime (&w_data.date) <= time)
    return;
  time = mktime (&w_data.date);

  fprintf(outfile,
      "%d:"               /* time */
      "%.1f:%.1f:%.1f:"   /* indoor temp */
      "%.1f:%.1f:%.1f:"   /* outdoor temp */
      "%.1f:%.1f:%.1f:"   /* outdoor temp 2 */
      "%.1f:%.1f:%.1f:%.1f:"   /* wind */
      "%.0f:"             /* pressure */
      "%.1f:%.1f:%.1f:" /* rain */
      "%d"              /* UV */
      "\n",

      (int)time,
 
      w_data.temp[0],
      w_data.humidity[0],
      w_data.dew_point[0],

      w_data.temp[1],
      w_data.humidity[1],
      w_data.dew_point[1],

      w_data.temp[2],
      w_data.humidity[2],
      w_data.dew_point[2],

      w_data.wind_direction,
      w_data.wind_gust,
      w_data.wind_average,
      w_data.wind_chill,

      w_data.absolute_pressure,
      /* w_data.relative_pressure, */

      w_data.rain_rate,
      w_data.rain_last_hour,
      w_data.rain_last_24hours,
      w_data.uv_index);

}

/* print human readable record */
void print_hrecord (FILE *outfile)
{
  char time[100];
  char *w_str;
  char **forecast_image;
  char **forecast_string;

  /* sanity check the date */
  if (w_data.date.tm_mday > 31) return;
  if (w_data.date.tm_mon > 12) return;

  if (w_data.date.tm_hour > 19 || w_data.date.tm_hour < 5)
    {
      forecast_image = forecast_img_night;
      forecast_string = forecast_str_night;
    }
  else
    {
      forecast_image = forecast_img_day;
      forecast_string = forecast_str_day;
    }
  
  strftime (time, 100, "%a, %d %b %Y &nbsp; %T %z", &w_data.date);
  w_str = wind_str (w_data.wind_direction);


  fprintf(outfile,
      "<b>Observations</b><br>%s</b><br><br>"               /* time */
      "<table><tr><th colspan=2>Temperature<th>Humidity<th>Dew Point"
      "<tr><th>Indoor  <td>%.1f&deg;C (%s)<td>%.1f%% (%s)<td>%.1f&deg;C" /* indoor temp */
      "<tr><th>Outdoor <td>%.1f&deg;C (%s)<td>%.1f%% (%s)<td>%.1f&deg;C" /* outdoor temp */
      "<tr><th>Temp 2<td>%.1f&deg;C (%s)<td>%.1f%% (%s)<td>%.1f&deg;C" /* temp sensor 2 */
      "</table><br>"
      "<table><tr><th>Wind<td>%.1f&deg; (%s)<td>%.1f <small><i>gust</i></small><td>%.1f <small><i>average</i></small><td>%.1f&deg;C <small><i>chill</i></small>"   /* wind */
      "<tr><th>Rain<td>%.1f<small>mm/hr</small><td>%.1f<small><i>mm/last hour</i></small><td>%.1f <small><i>mm/last 24 hours</i></small>" /* rain */
      "<tr><th>Pressure<td>%.0f<small>mb</small><th>Forecast<td>%s %s"             /* pressure */
      "<tr><th>UV<td>%d"
      "</table>",

      time,

      w_data.temp[0],
      trend [w_data.temp_trend[0]],
      w_data.humidity[0],
      trend [w_data.humidity_trend[0]],
      w_data.dew_point[0],

      w_data.temp[1],
      trend [w_data.temp_trend[1]],
      w_data.humidity[1],
      trend [w_data.humidity_trend[1]],
      w_data.dew_point[1],

      w_data.temp[2],
      trend [w_data.temp_trend[2]],
      w_data.humidity[2],
      trend [w_data.humidity_trend[2]],
      w_data.dew_point[2],

      w_data.wind_direction,
      w_str,
      w_data.wind_gust,
      w_data.wind_average,
      w_data.wind_chill,

      w_data.rain_rate,
      w_data.rain_last_hour,
      w_data.rain_last_24hours,

      w_data.absolute_pressure,
      /* w_data.relative_pressure, */
      forecast_string[w_data.forecast],
      forecast_image[w_data.forecast],

      w_data.uv_index
);


  g_free (w_str);
}

#define DATAPATH "/home/weather/data"
#define INPUTFILE "/home/weather/data/log.bin"
#define RECSIZE sizeof(unsigned char)

int
main(int argc, char **argv)
{
  FILE *bin;
  FILE *outfile;
  int data;
  int last_data;
  unsigned char rec[24];
  struct stat info;
  int rec_read = 0;

  stat (INPUTFILE, &info);
  bin = fopen (INPUTFILE, "r");

  if (!bin)
  {
    printf ("Error opening log file");
    return 1;
  }


  outfile = fopen (DATAPATH "/weather.txt", "w");

  fprintf (outfile, "#time:temp0:hum0:dew0:"
                    "temp1:hum1:dew1:"
                    "wind-direction:gust:wind-average:wind-chill:"
                    "pressure:"
                    "rain-rate:rain-last-hour:rain-last-24hrs\n");

  printf ("Reading...\n");

  for (data = fgetc (bin); !ferror (bin) && !feof (bin); data = fgetc (bin))
  {
    int idx = 0;

    if (ftell(bin) % 50000 == 0)
      fprintf (stderr, "\r%.2f%% (%d)",
          (float)ftell (bin)/(float)info.st_size * 100, (int) ftell (bin));

    /* find the begining of a record */
    if (!(data == 0xff && last_data == 0xff))
    {
      last_data = data;
      continue;
    }

    rec_read++;

    data = 0;
    last_data = 0;

    /* build up a new data record, the first two bytes are needed to
     * determine how long the record will be */
    if (!fread (rec, RECSIZE, 2, bin)) continue;

    switch (rec[1])
    {
      case DATE:
        if (!fread (&rec[2], RECSIZE, 10, bin))
          continue;

        w_data.date.tm_sec = 0;
        w_data.date.tm_min = rec[4];
        w_data.date.tm_hour = rec[5];
        w_data.date.tm_mday = rec[6];
        w_data.date.tm_mon = rec[7]-1;
        w_data.date.tm_year = rec[8] + 2000 - 1900;
        w_data.date.tm_gmtoff = rec[9];
        w_data.date.tm_isdst = 0;

        w_data.date.tm_wday = 0;
        w_data.date.tm_yday = 0;
        w_data.date.tm_zone = NULL;

        print_record (outfile);
        break;

      case TEMP:
        if (!fread (&rec[2], RECSIZE, 10, bin))
          continue;

        idx = rec[2] & 0xf;

        if (idx > 3)
          continue;

        int tmp;
        tmp = (rec[4] * 256 + rec[3]);
        if (tmp & 0x8000)
        {
          tmp = tmp & 0x7fff;
          tmp = tmp * -1;
        }
        w_data.temp[idx] = tmp / 10.0;


        w_data.humidity[idx] = rec[5];

        tmp = (rec[7] * 256 + rec[6]);
        if (tmp & 0x8000)
        {
          tmp = tmp & 0x7fff;
          tmp = tmp * -1;
        }
        w_data.dew_point[idx] = tmp / 10.0;
        w_data.humidity_trend[idx] = (rec[2] & 0x18) >> 4;
        w_data.temp_trend[idx] = rec[0] & 0x3;

        break;

      case WIND:
        if (!fread (&rec[2], RECSIZE, 9, bin))
          continue;

        w_data.wind_direction = (rec[2] & 0xf) * 360 / 16;
        w_data.wind_gust = ((rec[5] & 0xf) * 256 + rec[4]) / 10.0;
        w_data.wind_average = (((rec[5] & 0xf0) >> 4) + rec[6] * 16) / 10.0;

        /* convert from m/s to mph */
        w_data.wind_gust = w_data.wind_gust * 2.237;
        w_data.wind_average = w_data.wind_average * 2.237;

        if ((rec[7] & 0xf0 >> 4) == 1)
        {
          w_data.wind_chill = (256 * (rec[8] & 0xf) + rec[7]) / 10.0;
        }

        break;

      case PRESSURE:
        if (!fread (&rec[2], RECSIZE, 6, bin))
          continue;
        w_data.absolute_pressure = (rec[3] & 0xf) * 256 + rec[2];
        w_data.relative_pressure = (rec[5] & 0xf) * 256 + rec[4];
        w_data.forecast = (rec[3] & 0xf0) >> 4;

        break;

      case RAIN:
        if (!fread (&rec[2], RECSIZE, 15, bin))
          continue;

        w_data.rain_rate = (rec[3] * 256 + rec[2]) / 10.0;
        w_data.rain_last_hour = (rec[5] * 256 + rec[4]) / 10.0;
        w_data.rain_last_24hours = (rec[7] * 256 + rec[6]) / 10.0;
        w_data.total_rain = (rec[9] * 256 + rec[8]) / 10.0;
        w_data.rain_reset_date.tm_min = rec[10];
        w_data.rain_reset_date.tm_hour = rec[11];
        w_data.rain_reset_date.tm_mday = rec[12];
        w_data.rain_reset_date.tm_mon = rec[13];
        w_data.rain_reset_date.tm_year = rec[14] + 2000;

        break;

      case UV:
        if (!fread (&rec[2], RECSIZE, 6, bin))
          continue;
        w_data.uv_index = rec[3];
        break;
      default:
        printf ("Unknown record received (%x)\n", rec[1]);
    }
  }

  if (ferror(bin))
    printf ("\nError reading log file\n");

  if (feof (bin))
    printf ("\nFinished reading log file\n");

  fclose (bin);

  printf ("%d records read.\n", rec_read);

  fclose (outfile);

  outfile = fopen (DATAPATH "/current-reading.html", "w");
  print_hrecord (outfile);
  fclose (outfile);


  return 0;
}
