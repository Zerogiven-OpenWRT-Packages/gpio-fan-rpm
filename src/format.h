#ifndef FORMAT_H
#define FORMAT_H

// Format RPM as numeric string (with newline)
char* format_numeric(double rpm);

// Format RPM and GPIO as JSON string (with newline)
char* format_json(int gpio, double rpm);

// Format RPM and GPIO as collectd PUTVAL string (with newline)
char* format_collectd(int gpio, double rpm, int duration);

#endif // FORMAT_H