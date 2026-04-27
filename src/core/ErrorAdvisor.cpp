#include "ErrorAdvisor.h"

Diagnosis ErrorAdvisor::advise(const QString &raw)
{
    Diagnosis d;
    if (raw.isEmpty()) return d;
    const QString m = raw.toLower();
    d.isError = true;
    d.severity = "warning";

    if (m.contains("no such command or directory (wireless)") ||
        m.contains("no such command (wireless)")) {
        d.title = "Нет пакета wireless";
        d.cause = "На устройстве RouterOS 7 пакета wireless нет — используется новый стек wifi.";
        d.fix   = "Используйте команды /interface/wifi/ вместо /interface/wireless/. "
                  "В диалоге Wi-Fi выберите интерфейс wifi1/wifi2 вместо wlan1.";
        return d;
    }
    if (m.contains("no such command prefix") || m.contains("bad command name")) {
        d.title = "Команда не распознана";
        d.cause = "RouterOS не понимает указанный путь команды.";
        d.fix   = "Проверьте написание (например /ip/address/print, обратите внимание на слэши). "
                  "На разных версиях ROS пути могут отличаться.";
        return d;
    }
    if (m.contains("no such item")) {
        d.title = "Объект не найден";
        d.cause = "Указанный элемент (интерфейс/правило/IP) не существует.";
        d.fix   = "Сначала проверьте что объект есть: /interface/print или /ip/address/print.";
        return d;
    }
    if (m.contains("already have such address") || m.contains("address already in use")) {
        d.title = "IP уже назначен";
        d.cause = "Этот IP-адрес уже привязан к интерфейсу.";
        d.fix   = "Удалите старую запись или используйте другой IP. "
                  "Посмотреть назначенные адреса: /ip/address/print.";
        return d;
    }
    if (m.contains("expected end of command")) {
        d.title = "Лишние параметры";
        d.cause = "Команда содержит больше параметров, чем ожидалось.";
        d.fix   = "Проверьте что не указали несовместимые ключи одной командой.";
        return d;
    }
    if (m.contains("invalid value") || m.contains("input does not match any value")) {
        d.title = "Недопустимое значение параметра";
        d.cause = "Один из параметров получил значение, которое RouterOS не принимает.";
        d.fix   = "Проверьте формат: например IP с маской 192.168.1.1/24, MAC через двоеточия 00:11:22:...";
        return d;
    }
    if (m.contains("login failure") || m.contains("invalid user")) {
        d.title = "Неверный логин или пароль";
        d.cause = "RouterOS отклонил аутентификацию.";
        d.fix   = "Проверьте поля User/Password. По умолчанию пользователь admin без пароля.";
        return d;
    }
    if (m.contains("timeout") || m.contains("connection refused")) {
        d.title = "Нет ответа от устройства";
        d.cause = "Устройство недоступно по сети или API-сервис выключен.";
        d.fix   = "Убедитесь что включён сервис /ip/service/enable api (порт 8728). "
                  "Проверьте что нет файрвола между вами и роутером.";
        return d;
    }
    if (m.contains("not enough permissions")) {
        d.title = "Недостаточно прав";
        d.cause = "Учётная запись не имеет доступа к этой команде.";
        d.fix   = "Используйте пользователя группы full или назначьте нужные политики в /user/group.";
        return d;
    }
    if (m.contains("failure:")) {
        d.title = "Ошибка выполнения";
        d.cause = raw.section("failure:", 1).trimmed();
        d.fix   = "Подсказка: загляните в /log/print — там будет подробность от RouterOS.";
        return d;
    }
    d.isError = false;
    return d;
}
