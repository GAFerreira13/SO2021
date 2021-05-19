cmd_/home/goncalof/host/lab2/echo/modules.order := {   echo /home/goncalof/host/lab2/echo/echo.ko; :; } | awk '!x[$$0]++' - > /home/goncalof/host/lab2/echo/modules.order
