/* BSD non-blocking socket example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sys/socket.h"
#include "netdb.h"
#include "errno.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "protocol_examples_common.h"

#include "esp_wifi.h"
#include "esp_event.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include <lwip/netdb.h>

#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

#include "regex.h"
/**
 * @brief Indicates that the file descriptor represents an invalid (uninitialized or closed) socket
 *
 * Used in the TCP server structure `sock[]` which holds list of active clients we serve.
 */
/*TCP-SERVER*/
#define INVALID_SOCK (-1)
/**
 * @brief Time in ms to yield to all tasks when a non-blocking socket would block
 *
 * Non-blocking socket operations are typically executed in a separate task validating
 * the socket status. Whenever the socket returns `EAGAIN` (idle status, i.e. would block)
 * we have to yield to all tasks to prevent lower priority tasks from starving.
 */
/*TCP-SERVER*/
#define YIELD_TO_ALL_MS 10//50
#define CONFIG_EXAMPLE_TCP_SERVER_BIND_ADDRESS "0.0.0.0"
#define CONFIG_EXAMPLE_TCP_SERVER_BIND_PORT "2020"
#define EXAMPLE_TCP_SERVER  n
/*END*/

/*WIFI*/
#define EXAMPLE_ESP_WIFI_SSID  "AGV"      //"CTY ROBOTICS AUBOT"
#define EXAMPLE_ESP_WIFI_PASS      "123456789"
#define EXAMPLE_ESP_MAXIMUM_RETRY   10

#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD   WIFI_AUTH_WEP
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER ""

/*END*/

/*UART*/
#define ECHO_TEST_TXD (17)
#define ECHO_TEST_RXD (16)
#define ECHO_TEST_RTS (-1)
#define ECHO_TEST_CTS (-1)

#define ECHO_UART_PORT_NUM      (2)
#define ECHO_UART_BAUD_RATE     (57600)
#define ECHO_TASK_STACK_SIZE    (2048)

static const char *TAG0 = "UART TEST";
esp_err_t retval = ESP_OK;

#define BUF_SIZE (1024)
static void echo_task_config(void);
static void rev_data(const int sock);
static void trans_data(char *data , int len);
//static  wifi_find_strongest_bssid();
/*E-UART*/

/*chuyen vung*/
#define DEFAULT_SCAN_LIST_SIZE     5 //CONFIG_EXAMPLE_SCAN_LIST_SIZE 10

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define WIFI_STATED_BIT BIT2
#define WIFI_DISCONNECT_BIT BIT3
#define WIFI_SCAN_DONE_BIT BIT4

bool disconnect_flag = false ;
static EventGroupHandle_t s_wifi_event_group;
static const char *TAG = "scan";
static const char *TAG1 = "wifi station";
static int s_retry_num = 0;

/*DOIMANG*/
wifi_config_t wifi_config = 
{
        .sta = 
        {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
        },
}   ;

    wifi_scan_config_t scan_config =
{
    //.ssid = "CTY ROBOTICS AUBOT",
    .scan_type = WIFI_SCAN_TYPE_ACTIVE,
    .scan_time = 
    {
        .active.min = 0,
        .active.max = 60,
    }
};
/*DOIMANG*/
/*E-Chuyenvung*/
/*setup event wifi*/
void disconnect_wifi(void)
{
    ESP_ERROR_CHECK(esp_wifi_disconnect());
    disconnect_flag = true;
}
void scan_wifi(void)
{
    uint8_t *pt = (uint8_t*)"CTY ROBOTICS AUBOT";
    scan_config.ssid = pt;
    // memcpy(pt,"CTY ROBOTICS AUBOT",strlen(EXAMPLE_ESP_WIFI_SSID));
    // scan_config.ssid = pt;
    retval = esp_wifi_scan_start(&scan_config,false); //retval = esp_wifi_scan_start(NULL,false);
    ESP_LOGI("SCAN DONE","Status scan: %s",esp_err_to_name(retval));
}
static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data) // ham xu ly nhom su kien
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) 
    {
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) 
    {
        if(disconnect_flag)
        {
          xEventGroupSetBits(s_wifi_event_group, WIFI_DISCONNECT_BIT);
          disconnect_flag = false;
        }
        else
        {
            if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) 
            {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG1, "retry to connect to the AP");
            } 
            else 
            {
                xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            }
        }
        ESP_LOGI(TAG1,"connect to the AP fail");
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_SCAN_DONE)
    {
        xEventGroupSetBits(s_wifi_event_group, WIFI_SCAN_DONE_BIT);  
    } 
}
    /*****************************************************************ANH TU SUA CODE*/

    // if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    // {
    //     ESP_LOGI(TAG1, "Wifi Driver has stated");
    //     xEventGroupSetBits(s_wifi_event_group, WIFI_STATED_BIT);
    // }
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_STOP)
    // {
    //     ESP_LOGI(TAG1, "Wifi Driver has stopped");
    //     xEventGroupClearBits(s_wifi_event_group, WIFI_STATED_BIT);
    // }
    // else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    // {
    //     ESP_LOGI(TAG1, "Wifi Disconected");
    //     xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    // }
    // else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    // {
    //     ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
    //     ESP_LOGI(TAG1, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    //     xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    // }
// }
/*CONFIG WIFI DOI MAANG*/
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate(); // Khoi tao nhom su kien
    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "wifi_init_sta finished."); 

}

wifi_ap_record_t ap_info_connect; // luu thong tin AP sau moi lan quet
int rssi_connect = -90;
static void scan_wifi_task(void *pvParameters)
{
    while(1)
    {
        /*CODE NEU DUNG GIOI HAN RSSI PHAI QUET MOI VONG*/
         //if(esp_wifi_sta_get_ap_info(&ap_info_connect) == ESP_OK)
        // {
        //     ESP_LOGI("RSSI_CONNECT_BEFORE_SCAN","rssi: %d",ap_info_connect.rssi);
        //     ESP_LOGI("BSSID_CONNECT_BEFORE_SCAN","bssid: %2x:%2x:%2x:%2x:%2x:%2x",ap_info_connect.bssid[0],ap_info_connect.bssid[1],ap_info_connect.bssid[2],ap_info_connect.bssid[3],ap_info_connect.bssid[4],ap_info_connect.bssid[5]);
        // }
       
        // if((ap_info_connect.rssi < -65)||(ap_info_connect.rssi == 0))
        // {
        /*************************************************/
        //////////
        // if(esp_wifi_sta_get_ap_info(&ap_info_connect) == ESP_OK )
        // {
        //     ESP_LOGI("RSSI_CONNECT_BEFORE_SCAN","rssi: %d",ap_info_connect.rssi);
        //     ESP_LOGI("BSSID_CONNECT_BEFORE_SCAN","bssid: %2x:%2x:%2x:%2x:%2x:%2x",ap_info_connect.bssid[0],ap_info_connect.bssid[1],ap_info_connect.bssid[2],ap_info_connect.bssid[3],ap_info_connect.bssid[4],ap_info_connect.bssid[5]);
        // }
        //////
            uint16_t number = DEFAULT_SCAN_LIST_SIZE;
            wifi_ap_record_t ap_info[DEFAULT_SCAN_LIST_SIZE];
            uint16_t ap_count = 0;         
            memset(ap_info, 0, sizeof(ap_info));
            scan_wifi();
            //esp_wifi_scan_start(NULL, false);
            xEventGroupWaitBits(s_wifi_event_group, // cho xet co de xu ly su kien quet, no se cho den khi nao co WIFI_SCAN_DONE_BIT duoc bat len
                        WIFI_SCAN_DONE_BIT,
                        pdTRUE,  
                        pdFALSE,
                        portMAX_DELAY);
            ESP_LOGI(TAG, "Max AP number ap_info can hold = %u", number);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_num(&ap_count));
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));
            ESP_LOGI(TAG, "Total APs scanned = %u, actual AP number ap_info holds = %u", ap_count, number);
            int max_rssi = -90;
            for (int i = 0; i < number; i++) 
            {
                if(memcmp(ap_info[i].bssid,ap_info_connect.bssid,6) == 0) // lay RSSSI cua AP dang connect
                    {
                        rssi_connect = ap_info[i].rssi;
                        ESP_LOGI("RSSI","rssi: %d",rssi_connect);  
                    }
                if(memcmp(ap_info[i].ssid,EXAMPLE_ESP_WIFI_SSID,strlen(EXAMPLE_ESP_WIFI_SSID)) == 0) // Tim cac AP co cung SSID va pasword de so sanh rssi
                {
                    if(ap_info[i].rssi - rssi_connect > 10) // so sanh khoang rssi neu lon hon 10
                    {
                        if(ap_info[i].rssi > max_rssi) // tim max rssi 
                    {
                        max_rssi = ap_info[i].rssi; 
                        memcpy(wifi_config.sta.bssid,ap_info[i].bssid,6);// luu vao wifi_congfig de cau hinh lai trong esp_wifi_set_config
                    }
                    }
                }
            }
            ESP_LOGI("BSSID_MAX","bssid :%2x:%2x:%2x:%2x:%2x:%2x",wifi_config.sta.bssid[0],wifi_config.sta.bssid[1],wifi_config.sta.bssid[2],wifi_config.sta.bssid[3],wifi_config.sta.bssid[4],wifi_config.sta.bssid[5]);
            if(memcmp(ap_info_connect.bssid,wifi_config.sta.bssid,6)!= 0) // lay wifi_config lam chuan de so sanh voi bssid dang conncet
            {
                disconnect_wifi(); // ham disconnect wifi
                ESP_LOGI("Status","START DISCONNECT");
                /*EventBit for DISCONNECT_WIFI*/
                EventBits_t bit = xEventGroupWaitBits(s_wifi_event_group, // cho xet co de xu ly
                        WIFI_DISCONNECT_BIT,
                        pdFALSE,  
                        pdFALSE,
                        portMAX_DELAY);
                s_retry_num = EXAMPLE_ESP_MAXIMUM_RETRY;     // dat lai so lan ket noi de tranh conncet lai trong event handler
                if(bit & WIFI_DISCONNECT_BIT)
                {
                    ESP_LOGI("Status","DISCONNECT SUCCESS");
                    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config)); // set lai config cho wifi theo bssid moi
                    ESP_LOGI("Status","CONFIG WIFI SUCCESS");
                    ESP_ERROR_CHECK(esp_wifi_connect()); // ham connect lai wifi
                    vTaskDelay(10/ portTICK_PERIOD_MS);
                    ESP_LOGI("Status","CHUYEN WIFI THANH CONG");  
                }       
            }
            /*EventBit for CONNECT_WIFI*/
            EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,  // cho set co de xu ly (co nay duoc set len trong eventhandler)
                    WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                    pdFALSE,
                    pdFALSE,
                    portMAX_DELAY); // delay neu co chua duoc set len

            if (bits & WIFI_CONNECTED_BIT) 
            {
                ESP_LOGI(TAG1, "connected to ap SSID:%s password:%s",
                        EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
                if (esp_wifi_sta_get_ap_info(&ap_info_connect) == ESP_OK) 
                {
                /*In ra BSSID (Ä‘á»‹a chá»‰ MAC cá»§a AP)*/
                memcpy(wifi_config.sta.bssid,ap_info_connect.bssid,6);
                ESP_LOGI(TAG, "rssi:%d, bssid: %2x:%2x:%2x:%2x:%2x:%2x",
                        ap_info_connect.rssi, ap_info_connect.bssid[0],ap_info_connect.bssid[1],ap_info_connect.bssid[2],ap_info_connect.bssid[3],ap_info_connect.bssid[4],ap_info_connect.bssid[5]);
                }
            } 
            else if (bits & WIFI_FAIL_BIT) 
            {
                ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                        EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
            } 
            else 
            {
                ESP_LOGE(TAG, "UNEXPECTED EVENT");
            }

            vTaskDelay(5000/ portTICK_PERIOD_MS);
        }
        // else
        // {
        //    ESP_LOGI("Status","KHONG CO SU THAY DOI VE WIFI");
        // }
        
    //}
}
/*DOI MANG O DAY*/
/*RTOS*/
#define QUEUE_SIZE 10
#define DATA_SIZE 512
QueueHandle_t dataQueue;
static const char *TAG3 = "UART QUEUE";

 static void echo_task_config(void)
{
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = 
    {
        .baud_rate = ECHO_UART_BAUD_RATE, // 
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    int intr_alloc_flags = 0;

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));
}
static void trans_data(char *data , int len)  
{
        // Write data back to the UART
        uart_write_bytes(ECHO_UART_PORT_NUM, data,len);//(const char *) data
         if (len) 
         {
             data[len] = '\0';
             //ESP_LOGI(TAG0, "Recv str: %s", (char *) data);
             ESP_LOG_BUFFER_HEXDUMP(TAG0, data, len, ESP_LOG_INFO);
         }
}
static void rev_data(const int sock)
{
    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
        // Read data from the UART
        int len = uart_read_bytes(ECHO_UART_PORT_NUM, data,(BUF_SIZE - 1), 20 / portTICK_PERIOD_MS); //(BUF_SIZE - 1)
        send(sock,data,len,0); // ham truyen lai
         if (len)
          {
              data[len] = '\0';
              ESP_LOGI(TAG0, "Recv str: %s", (char *) data);
          }
}
/*END*/
/**
 * @brief Utility to log socket errors
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket number
 * @param[in] err Socket errno
 * @param[in] message Message to print
 */
static void log_socket_error(const char *tag, const int sock, const int err, const char *message)
{
    ESP_LOGE(tag, "[sock=%d]: %s\n"
                  "error=%d: %s", sock, message, err, strerror(err));
}

/**
 * @brief Tries to receive data from specified sockets in a non-blocking way,
 *        i.e. returns immediately if no data.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket for reception
 * @param[out] data Data pointer to write the received data
 * @param[in] max_len Maximum size of the allocated space for receiving data
 * @return
 *          >0 : Size of received data
 *          =0 : No data available
 *          -1 : Error occurred during socket read operation
 *          -2 : Socket is not connected, to distinguish between an actual socket error and active disconnection
 */
static int try_receive(const char *tag, const int sock, char * data, size_t max_len)
{
    int len = recv(sock, data, max_len, 0);
    if (len < 0) 
    {
        if (errno == EINPROGRESS || errno == EAGAIN || errno == EWOULDBLOCK) 
        {
            return 0;   // Not an error
        }
        if (errno == ENOTCONN) 
        {
            ESP_LOGW(tag, "[sock=%d]: Connection closed", sock);
            return -2;  // Socket has been disconnected
        }
        log_socket_error(tag, sock, errno, "Error occurred during receiving");
        return -1;
    }

    return len;
}

/**
 * @brief Sends the specified data to the socket. This function blocks until all bytes got sent.
 *
 * @param[in] tag Logging tag
 * @param[in] sock Socket to write data
 * @param[in] data Data to be written
 * @param[in] len Length of the data
 * @return
 *          >0 : Size the written data
 *          -1 : Error occurred during socket write operation
 */
static int socket_send(const char *tag, const int sock, const char * data, const size_t len)
{
    int to_write = len;
    while (to_write > 0) 
    {
        int written = send(sock, data + (len - to_write), to_write, 0);
        if (written < 0 && errno != EINPROGRESS && errno != EAGAIN && errno != EWOULDBLOCK) 
        {
            log_socket_error(tag, sock, errno, "Error occurred during sending");
            return -1;
        }
        to_write -= written;
    }
    return len;
}

/**
 * @brief Returns the string representation of client's address (accepted on this server)
 */
static inline char* get_clients_address(struct sockaddr_storage *source_addr)
{
    static char address_str[128];
    char *res = NULL;
    // Convert ip address to string
    if (source_addr->ss_family == PF_INET) 
    {
        res = inet_ntoa_r(((struct sockaddr_in *)source_addr)->sin_addr, address_str, sizeof(address_str) - 1);
    }
#ifdef CONFIG_LWIP_IPV6
    else if (source_addr->ss_family == PF_INET6)
    {
        res = inet6_ntoa_r(((struct sockaddr_in6 *)source_addr)->sin6_addr, address_str, sizeof(address_str) - 1);
    }
#endif
    if (!res) 
    {
        address_str[0] = '\0'; // Returns empty string if conversion didn't succeed
    }
    return address_str;
}

static void tcp_server_task(void *pvParameters)
{
    char data[512];
    static char rx_buffer[512];
    static const char *TAG = "nonblocking-socket-server";
    SemaphoreHandle_t *server_ready = pvParameters;
    struct addrinfo hints = { .ai_socktype = SOCK_STREAM };
    struct addrinfo *address_info;
    int listen_sock = INVALID_SOCK;
    const size_t max_socks = CONFIG_LWIP_MAX_SOCKETS - 1;
    static int sock[CONFIG_LWIP_MAX_SOCKETS - 1];

    // Prepare a list of file descriptors to hold client's sockets, mark all of them as invalid, i.e. available
    for (int i=0; i<max_socks; ++i) 
    {
        sock[i] = INVALID_SOCK;
    }
    // Translating the hostname or a string representation of an IP to address_info
    int res = getaddrinfo(CONFIG_EXAMPLE_TCP_SERVER_BIND_ADDRESS, CONFIG_EXAMPLE_TCP_SERVER_BIND_PORT, &hints, &address_info);
    if (res != 0 || address_info == NULL) 
    {
        ESP_LOGE(TAG, "couldn't get hostname for `%s` "
                      "getaddrinfo() returns %d, addrinfo=%p", CONFIG_EXAMPLE_TCP_SERVER_BIND_ADDRESS, res, address_info);
        goto error;
    }

    // Creating a listener socket
    listen_sock = socket(address_info->ai_family, address_info->ai_socktype, address_info->ai_protocol);

    if (listen_sock < 0) 
    {
        log_socket_error(TAG, listen_sock, errno, "Unable to create socket");
        goto error;
    }
    ESP_LOGI(TAG, "Listener socket created");

    // Marking the socket as non-blocking
    int flags = fcntl(listen_sock, F_GETFL);
    if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) 
    {
        log_socket_error(TAG, listen_sock, errno, "Unable to set socket non blocking");
        goto error;
    }
    ESP_LOGI(TAG, "Socket marked as non blocking");

    // Binding socket to the given address
    int err = bind(listen_sock, address_info->ai_addr, address_info->ai_addrlen);
    if (err != 0) 
    {
        log_socket_error(TAG, listen_sock, errno, "Socket unable to bind");
        goto error;
    }
    ESP_LOGI(TAG, "Socket bound on %s:%s", CONFIG_EXAMPLE_TCP_SERVER_BIND_ADDRESS, CONFIG_EXAMPLE_TCP_SERVER_BIND_PORT);

    // Set queue (backlog) of pending connections to one (can be more)
    err = listen(listen_sock, 1);
    if (err != 0) 
    {
        log_socket_error(TAG, listen_sock, errno, "Error occurred during listen");
        goto error;
    }
    ESP_LOGI(TAG, "Socket listening");
    xSemaphoreGive(*server_ready);

    // Main loop for accepting new connections and serving all connected clients
    while (1) 
    {
        struct sockaddr_storage source_addr; // Large enough for both IPv4 or IPv6
        socklen_t addr_len = sizeof(source_addr);

        // Find a free socket
        int new_sock_index = 0;
        for (new_sock_index=0; new_sock_index<max_socks; ++new_sock_index) 
        {
            if (sock[new_sock_index] == INVALID_SOCK) 
            {
                break;
            }
        }

        // We accept a new connection only if we have a free socket
        if (new_sock_index < max_socks) 
        {
            // Try to accept a new connections
            sock[new_sock_index] = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);

            if (sock[new_sock_index] < 0) 
            {
                if (errno == EWOULDBLOCK) 
                { // The listener socket did not accepts any connection
                                            // continue to serve open connections and try to accept again upon the next iteration
                    ESP_LOGV(TAG, "No pending connections...");
                } 
                else 
                {
                    log_socket_error(TAG, listen_sock, errno, "Error when accepting connection");
                    goto error;
                }
            } 
            else 
            {
                // We have a new client connected -> print it's address
                ESP_LOGI(TAG, "[sock=%d]: Connection accepted from IP:%s", sock[new_sock_index], get_clients_address(&source_addr));

                // ...and set the client's socket non-blocking
                flags = fcntl(sock[new_sock_index], F_GETFL);
                if (fcntl(sock[new_sock_index], F_SETFL, flags | O_NONBLOCK) == -1) 
                {
                    log_socket_error(TAG, sock[new_sock_index], errno, "Unable to set socket non blocking");
                    goto error;
                }
                ESP_LOGI(TAG, "[sock=%d]: Socket marked as non blocking", sock[new_sock_index]);
            }
        }

        // We serve all the connected clients in this loop
        for (int i=0; i<max_socks; ++i) 
        {
            if (sock[i] != INVALID_SOCK)
             {
                // This is an open socket -> try to serve it
                int len = try_receive(TAG, sock[i], rx_buffer, sizeof(rx_buffer));
                if (len < 0) 
                {
                    // Error occurred within this client's socket -> close and mark invalid
                    ESP_LOGI(TAG, "[sock=%d]: try_receive() returned %d -> closing the socket", sock[i], len);
                    close(sock[i]);
                    sock[i] = INVALID_SOCK;
                } 
                else if (len > 0) 
                {
                    // Received some data -> echo back
                    //ESP_LOGI(TAG, "[sock=%d]: Received %.*s", sock[i], len, rx_buffer);
                    ESP_LOG_BUFFER_HEXDUMP(TAG, rx_buffer, len, ESP_LOG_INFO);
                    trans_data(rx_buffer, len); // ham gui data bang UART (client -> server -> UART)
                    /**/
                    // int lenn = uart_read_bytes(ECHO_UART_PORT_NUM,data,(BUF_SIZE - 1), 20 / portTICK_PERIOD_MS); //(BUF_SIZE - 1)
                     if (len < 0) {
                         // Error occurred on write to this socket -> close it and mark invalid
                        ESP_LOGI(TAG, "[sock=%d]: socket_send() returned %d -> closing the socket", sock[i], len);
                        close(sock[i]);
                        sock[i] = INVALID_SOCK;
                        memset(rx_buffer,512,512*8); // chinh sua;
                     } 
                     else 
                     {
                    //     // Successfully echoed to this socket
                    //     ESP_LOGI(TAG, "[sock=%d]: Written %.*s", sock[i], lenn,data);//rx_buffer
                     }
                }
            } // one client's socket
        } // for all sockets
        // 
        int lenn = uart_read_bytes(ECHO_UART_PORT_NUM,data,(BUF_SIZE - 1), 100 / portTICK_PERIOD_MS); //(BUF_SIZE - 1) (UART -> ess32)
        for (int i=0; i<max_socks; ++i) 
        {
            if (sock[i] != INVALID_SOCK) 
            {
                //if(xQueueReceive(dataQueue, data,0) == pdPASS) sua
                //{ sua
                        //char lenn = strlen(data); sua
                        lenn = socket_send(TAG, sock[i],data,lenn);//rx_buffer (UART -> esp32(server)-> client)
                        if (lenn < 0) {
                            // Error occurred on write to this socket -> close it and mark invalid
                            ESP_LOGI(TAG, "[sock=%d]: socket_send() returned %d -> closing the socket", sock[i], lenn);
                            close(sock[i]);
                            sock[i] = INVALID_SOCK;
                        } 
                        if(lenn > 0) 
                        {
                            // Successfully echoed to this socket
                            ESP_LOGI(TAG, "[sock=%d]: Written %.*s", sock[i], lenn,data);//rx_buffer
                        }
                //}
            }
        }
            // Yield to other tasks
        vTaskDelay(pdMS_TO_TICKS(YIELD_TO_ALL_MS));
    }

error:
    if (listen_sock != INVALID_SOCK) {
        close(listen_sock);
    }

    for (int i=0; i<max_socks; ++i) {
        if (sock[i] != INVALID_SOCK) {
            close(sock[i]);
        }
    }

    free(address_info);
    vTaskDelete(NULL);
}
/*UART-RTOS*/
//  void uart_send_task(void *pvParameters) {
//     char data[512];
//     while (1) {
//         int lenn = uart_read_bytes(ECHO_UART_PORT_NUM,data,(BUF_SIZE - 1), 100 / portTICK_PERIOD_MS); //(BUF_SIZE - 1)
//         if (lenn) {
//               data[lenn] = '\0';
//                 xQueueSend(dataQueue,data, portMAX_DELAY);
//              ESP_LOGI(TAG3, "Recv str: %s", (char *) data);
//          }
//         vTaskDelay(pdMS_TO_TICKS(YIELD_TO_ALL_MS));
//     }

// }
/**/

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG1, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
    /*F-WIFI*/
// Khá»Ÿi táº¡o FreeRTOS queue
    // dataQueue = xQueueCreate(QUEUE_SIZE, DATA_SIZE);
    // if (dataQueue == NULL) {
    //     ESP_LOGE(TAG3, "Failed to create queue");
    //     return;
    // }
    /*******/
    /*S-UART*/
    echo_task_config();
    xEventGroupWaitBits(s_wifi_event_group,  // doi cho den khi wifi start xong
                    WIFI_CONNECTED_BIT,
                    pdFALSE,
                    pdFALSE,
                    portMAX_DELAY); 
    //xTaskCreate(scan_wifi_task, "scan_send", 4096, NULL,5,NULL); //TASK DOI MANG
    //xTaskCreate(uart_send_task, "uart_send", 4096, NULL,6, NULL);
    SemaphoreHandle_t server_ready = xSemaphoreCreateBinary();
    assert(server_ready);
    xTaskCreate(tcp_server_task, "tcp_server", 4096, &server_ready, 5, NULL);
    xSemaphoreTake(server_ready, portMAX_DELAY);
    vSemaphoreDelete(server_ready);
    xTaskCreate(scan_wifi_task, "scan_send", 4096, NULL,5,NULL); //TASK DOI MANG
}