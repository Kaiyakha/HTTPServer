# HTTPServer
HTTP сервер, принимающий изображение в формате JPEG и отправляющий отзеркаленное изображение в аналогичной кодировке.
## Сборка
### Linux
`g++ Server.cpp http.cpp image.cpp -std=c++17 -o server.out`
### Windows
Сборка с помощью Visual Studio
## Запуск
Запуск осуществляется через терминал
### Linux
`./server.out <IP> <ПОРТ>`
### Windows
`server.exe <IP> <ПОРТ>`
## Описание
Сервер открывает TCP-сокет, привязывает его к указанному IP и начинает прослушивание через указанный порт.
После подключения клиента и получения сырых данных сервер парсит данные в соответствии со спецификацией HTTP.
Сервер настроен на обработку POST-запроса с закодированными в формате JPEG данными в теле.
В ответ на любые другие методы в запросе сервер отправляет ответ со статус-кодом 405 "Method Not Allowed".
Если метод соответствует ожиданиям сервера, но тело запроса отсутствует или превышает размер буфера (не более 32Мб, в зависимости от размера заголовка HTTP),
сервер отвечает статус-кодом 400 "Bad Request".
Сервер интерпретирует полученные в теле запроса данные как закодированное в формате JPEG изображение независимо от содержимого тела.
Если серверу не удаётся отобразить изображение, клиенту приходит ответ со статус-кодом 500 "Internal Server Error".
