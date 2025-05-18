// Подключаем заголовочные файлы проекта и стандартные библиотеки
#include "./mongoose/mongoose.h"           // Библиотека Mongoose (HTTP-сервер)
#include "./input/input.h"                 // Заголовок для функции read_file
#include "./constants/constants.h"         // Пути к ресурсам и MIME-типы
#include <stdlib.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <time.h>  

// Ограничения на допустимый ввод числа
#define MIN_NUMBER 1
#define MAX_NUMBER 10

// Функция логирования HTTP-запросов (время, метод, путь)
static void log_request(struct mg_http_message *hm) {
    time_t now = time(NULL);
    char tmbuf[64];
    struct tm *tm = localtime(&now);
    strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", tm);

    printf("[%s] %.*s %.*s\n",              // Вывод: [время] МЕТОД ПУТЬ
        tmbuf,
        (int)hm->method.len, hm->method.buf,
        (int)hm->uri.len, hm->uri.buf);
	   
	fflush(stdout);                        // Обеспечиваем немедленный вывод в stdout
}

// Генерирует HTML с таблицей умножения для переданного числа
static char *generate_multiplication_table(int number) {
    char *html = malloc(4096);             // Выделяем буфер под HTML
    if (!html) return NULL;

    // Начало HTML-страницы
    strcpy(html,
        "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">"
        "<title>Результат</title>"
        "<link rel=\"stylesheet\" href=\"styles.css\">"
        "</head><body><div class=\"table-container\">"
        "<h2 style=\"text-align:center;\">Таблица умножения для ");

    // Вставляем выбранное число
    char number_str[32];
    sprintf(number_str, "%d", number);
    strcat(html, number_str);
    strcat(html, "</h2><table>");

    // Строим таблицу умножения от 1 до 10
    for (int i = 1; i <= 10; ++i) {
        char row[128];
        sprintf(row, "<tr><td>%d × %d</td><td>%d</td></tr>", number, i, number * i);
        strcat(html, row);
    }

    // Завершаем HTML-код страницы
    strcat(html,
        "</table>"
        "<div style=\"text-align:center; margin-top: 1.5rem;\">"
        "<a href=\"/\" class=\"btn-return\">Вернуться</a>"
        "</div>"
        "</div></body></html>");

    return html;
}

// Парсит число из тела POST-запроса (формы)
static int parse_number(const struct mg_str *body, int *result) {
    char number_str[100];
    char *endptr = NULL;

    // Извлекаем параметр "number" из формы
    if (mg_http_get_var(body, "number", number_str, sizeof(number_str)) <= 0) {
        return 1; // параметр не найден или пустой
    }

    // Преобразуем в число
    long number = strtol(number_str, &endptr, 10);
    if (*endptr != '\0') return 2; // не число

    // Проверяем диапазон
    if (number < MIN_NUMBER || number > MAX_NUMBER) return 3;

    *result = (int) number;
    return 0; // OK
}

// Обрабатывает конкретный HTTP-запрос
static void process_request(struct mg_connection *c, struct mg_http_message *hm) {
    char *response = NULL;
    const char *ctype = CONTENT_TYPE_HTML;

    log_request(hm);  // Пишем в лог информацию о запросе

    // Обработка POST-запроса на /multiply
    if (!mg_strcmp(hm->uri, mg_str("/multiply")) && !mg_strcasecmp(hm->method, mg_str("POST"))) {
        int number = 0;
        int err = parse_number(&hm->body, &number);

        if (err == 0) {
            // Успешно — генерируем таблицу
            response = generate_multiplication_table(number);
            if (response) {
                mg_http_reply(c, 200, ctype, "%s", response);
                free(response);
            } else {
                mg_http_reply(c, 500, ctype, "Ошибка: не удалось выделить память.");
            }
        } else {
            // Ошибка ввода — читаем исходную HTML-страницу
            char *form_html = read_file(PATH_MULTIPLY_HTML);
            if (!form_html) {
                mg_http_reply(c, 500, ctype, "Ошибка загрузки страницы");
                return;
            }

            // Выбираем текст ошибки
            const char *error_message = NULL;
            if (err == 1) error_message = "Ошибка: параметр не передан.";
            else if (err == 2) error_message = "Ошибка: введите корректное число.";
            else if (err == 3) error_message = "Ошибка: число должно быть от 1 до 10.";
            else error_message = "Ошибка: неизвестная ошибка.";

            // Вставка ошибки в шаблон
            const char *error_template_fmt = "<div class=\"toast\">%s</div>";
            size_t error_len = strlen(error_template_fmt) + strlen(error_message) + 1;
            char *error_template = malloc(error_len);
            if (!error_template) {
                free(form_html);
                mg_http_reply(c, 500, ctype, "Ошибка выделения памяти");
                return;
            }
            snprintf(error_template, error_len, error_template_fmt, error_message);

            // Вставляем ошибку внутрь <body>
            char *body_pos = strstr(form_html, "<body>");
            if (body_pos) {
                body_pos += strlen("<body>");
                size_t new_size = strlen(form_html) + strlen(error_template) + 1;
                response = malloc(new_size);
                if (!response) {
                    free(form_html);
                    free(error_template);
                    mg_http_reply(c, 500, ctype, "Ошибка выделения памяти");
                    return;
                }
                snprintf(response, new_size, "%.*s%s%s",
                    (int)(body_pos - form_html), form_html,
                    error_template,
                    body_pos);

                mg_http_reply(c, 400, ctype, "%s", response);
                free(response);
            } else {
                // Если <body> не найден — просто возвращаем форму
                mg_http_reply(c, 400, ctype, "%s", form_html);
            }

            free(error_template);
            free(form_html);
        }
        return;
    }

    // Обработка запроса на /styles.css
    if (!mg_strcmp(hm->uri, mg_str("/styles.css"))) {
        response = read_file(PATH_CSS_STYLES);
        if (response) {
            ctype = CONTENT_TYPE_CSS;
            mg_http_reply(c, 200, ctype, "%s", response);
            free(response);
            return;
        }
        mg_http_reply(c, 404, "", "");
        return;
    }

    // Обработка остальных URI — возвращаем HTML-форму по умолчанию
    response = read_file(PATH_MULTIPLY_HTML);
    if (response) {
        mg_http_reply(c, 200, ctype, "%s", response);
        free(response);
    } else {
        mg_http_reply(c, 500, "", "");
    }
}

// Основной обработчик событий от сервера
static void main_fun(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_HTTP_MSG) {
        struct mg_http_message *hm = (struct mg_http_message *) ev_data;
        process_request(c, hm);  // Обрабатываем HTTP-запрос
    }
}

// Точка входа в программу
int main(void) {
    const char *server_address = "http://localhost:8081";  // Адрес сервера
    struct mg_mgr mgr;

    mg_mgr_init(&mgr);                         // Инициализация менеджера
    mg_http_listen(&mgr, server_address, main_fun, NULL);  // Запуск сервера
    printf("Сервер запущен на %s\n", server_address);

    // Главный цикл обработки событий
    for (;;) {
        mg_mgr_poll(&mgr, 1000);               // Периодическая проверка событий
    }

    mg_mgr_free(&mgr);                         // Очистка ресурсов
    return 0;
}
