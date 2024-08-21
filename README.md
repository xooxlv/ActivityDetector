# ActivityDetector

Для запуска клиента необходимо: 
	1) В директории с client.exe создать папку "screenshots"
	2) В директории с client.exe создать файл config.txt со следующим содержанием:
		
		control_server ip:port
		screenshot_dir screenshots

Где control_server - адрес и порт сервера, screenshot_dir - путь до созданной папки "screenshots" относительно файла client.exe