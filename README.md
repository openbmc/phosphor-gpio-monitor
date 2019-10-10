# phosphor-gpio-monitor

This is a daemon running within the OpenBMC environment. It accepts a command
line parameter for monitoring single gpio line and take action if requested.
This is an old implementation using event fd and only support single line
monitoring, for multiple lines, user has to run this daemon seperately for
each gpio line.

# phosphor-multi-gpio-monitor

This is a daemon running within the OpenBMC environment. It uses a well-defined
configuration file to monitor list of gpios from config file and take action
defined in config based on gpio state change. It uses libgpiod library.

# Difference
New implementation (gpioMonitor) provides multiple gpio line monitoring in
single instance of gpioMonitor running. It is very easy to add list of gpios
into JSON config file and it also supports of GPIO line by name defined in
kernel.

## Configuration

There is a gpioMonConfig.json file which defines details of GPIOs which is
required to be monitored. Following are fields in json file
1. Name: Name of gpio for reference.
2. LineName: this is the line name defined in device tree for specific gpio
3. GpioNum: GPIO offset, this field is optional if LineName is defined.
4. ChipId: This is device name either offset ("0") or complete gpio device
           ("gpiochip0"). This field is not required if LineName is defined.
5. EventMon: Event of gpio to be monitored. This can be "FALLING", "RISING"
             OR "BOTH". Default value for this is "BOTH".
6. Target: This is an optional systemd service which will get started after
           triggering event. A journal entry will be added for every event
           occurs irrespective of this definition.
7. Continue: This is a optional flag and if it is defined as true then this
             gpio will be monitored continously. If not defined then
             monitoring of this gpio will stop after first event.

## Sample config file

[
	{
		"Name": "PowerButton",
		"LineName": "POWER_BUTTON",
		"GpioNum": 34,
		"ChipId": "gpiochip0",
		"EventMon": "BOTH",
		"Continue": true
	},
	{
		"Name": "PowerGood",
		"LineName": "PS_PWROK",
		"EventMon": "FALLING",
		"Continue": false
	},
	{
		"Name": "SystemReset",
		"GpioNum": 46,
		"ChipId": "0"
	}
]
