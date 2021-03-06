## Обмен через сервисы windows

Compiled by Visual Studio 2006, project of 2008 year

## Порядок установки

* Скопировать установочные файлы
    * Файлы "Win1cService.exe" и "config.ini" в каталог "C:\Win1cService"
* Прописать настройки в файле настроек
    * Открыть файл "config.ini" и прописать подключение к базе, а также пользователя базы по правилам строки соединения.
    Например: 
    ```
    [Main]
    Function=Сервис_Функция <- это какая функция будет вызвана
    Base=File="F:\Base";Usr="Exchange"; <- это строка запуска
    ```

* Запустить инициализацию сервиса
    * Для этого в каталоге где находиться сервис "C:\Win1cService" нужно запустить сервис с параметром "install". 
    * Пример: 
    ```
    Win1cService.exe -install
    ```
* Настроить сервис в windows
    * Далее заходим в сервисы windows. Должен появится сервис "Win1c Exchange Server". По умолчанию он не запущен. Поэтому нужно настроить в соответствии с требованиями.
    * Рекомендуется сделать его автоматически запускаемым от имени системы.
* Проверить, что сервис работает
    * Заходим в 1С предприятие и смотрим в журнале событий что было COM подключение. Вызывается это подключение через период указанный в свойстве ПараметрыСеанса.ТекущийСклад.ПериодОпросаНаИзменения.

## Структура и настройки

* Каталог сервиса
    * Каталогом сервиса является папка откуда будет установлен сервис, нужно чтобы она была не сетевой и не содержала русских букв и без пробелов. Например: "C:\Win1cService".
* Сервис файл Win1cService.exe. Имеет два ключа запуска:
    * install - устанавливается в windows как сервис, после чего на этот файл ссылается сервис windows.
    * remove - удаляется из сервисов windows.
* Файл настроек config.ini состоит из двух параметров раздела [Main]
    * Правила строки соединения
    * "Base" это строка соединения 1С по правилам COM. Что можно писать в строке соединения можно прочитать в книжке 1С по администрированию, нас интересует только путь к базе и имя пользователя.
    * путь к базе прописывается как File="путь к базе";
    * имя пользователя прописывается как "Usr="имя пользователя";
    * Пароль в настройках специально не прописывается из условий безопасности. Поэтому пароль зашит в самом сервисе и проставляется автоматически при запуске сервиса.
* Сервисы
    * Function - Здесь обозначена какая функция будет вызвана.
* Настройки в программе 1С
    * Для работы нужно чтобы в был параметр сеанса ТекущийСклад со свойством **ПериодОпросаНаИзменения**.
