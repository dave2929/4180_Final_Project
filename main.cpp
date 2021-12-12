// ESP8266 Static page WEB server to control Mbed
 
#include "mbed.h"
 
Serial pc(USBTX, USBRX);
Serial esp(p28, p27); // tx, rx
 
DigitalOut myRGBled1R(p5);
DigitalOut myRGBled1G(p6);
DigitalOut myRGBled2R(p7);
DigitalOut myRGBled2G(p8);
DigitalOut myRGBled3R(p9);
DigitalOut myRGBled3G(p10);
DigitalOut myRGBled4R(p11);
DigitalOut myRGBled4G(p12);
PwmOut pump(p21);
PwmOut uv(p22);
AnalogIn waterLevel(p15);
AnalogIn waterQuality(p16);
// Standard Mbed LED definitions
//DigitalOut uv(p5);
//DigitalOut pump(p6);

 
/*
char ssid[32] = "hsd";     // enter WiFi router ssid inside the quotes
char pwd [32] = "austin123"; // enter WiFi router password inside the quotes
*/
float AdcIn, Ht;
float R1=100000, R2=10000; // resistor values to give a 10:1 reduction of measured AnalogIn voltage
char quality[10];
char level[10];
 
// things for sending/receiving data over serial
volatile int tx_in=0;
volatile int tx_out=0;
volatile int rx_in=0;
volatile int rx_out=0;
const int buffer_size = 4095;
char tx_buffer[buffer_size+1];
char rx_buffer[buffer_size+1];
void Tx_interrupt();
void Rx_interrupt();
void send_line();
void read_line();
 
int DataRX;
int update;
int count;
char cmdbuff[1024];
char replybuff[4096];
char webdata[4096]; // This may need to be bigger depending on WEB browser used
char webbuff[4096];     // Currently using 1986 characters, Increase this if more web page data added
char timebuf[30];
void SendCMD(),getreply(),ReadWebData(),startserver();
void gettime(),setRTC(),getlevel(),getquality();
char rx_line[1024];
int port        =80;  // set server port
int SERVtimeout =5;    // set server timeout in seconds in case link breaks.
struct tm t;
// manual set RTC values
int minute      =00;    // 0-59
int hour        =12;    // 2-23
int dayofmonth  =26;    // 1-31
int month       =8;     // 1-12
int year        =15;    // last 2 digits
 
int main()
{
    pc.baud(9600);
    esp.baud(9600);
    uv=1,pump=1;
    // Setup a serial interrupt function to receive data
    esp.attach(&Rx_interrupt, Serial::RxIrq);
    // Setup a serial interrupt function to transmit data
    esp.attach(&Tx_interrupt, Serial::TxIrq);
    if (time(NULL) < 1420070400) {
        setRTC();
    }
    startserver();
    DataRX=0;
    count=0;
    while(1) {
        if(DataRX==1) {
            ReadWebData();
            esp.attach(&Rx_interrupt, Serial::RxIrq);
        }
        if(update==1) // update time, hit count, and analog levels in the HUZZAH chip
        {
            // get new values
            gettime();
            getlevel();
            getquality();
            count++;
            // send new values
            sprintf(cmdbuff, "count,time,analog1,analog2=%d,\"%s\",\"%s\",\"%s\"\r\n",count,timebuf,level,quality);
            SendCMD();
            getreply();
            update=0;   
        }
        
        //water level is too low
        if (waterLevel <= 0.15) {
            myRGBled1R = 0;
            myRGBled1G = 0;
            myRGBled2R = 0;
            myRGBled2G = 0;
            myRGBled3R = 0;
            myRGBled3G = 0;
            myRGBled4R = 0;
            myRGBled4G = 0;
            pump = 0;  //close the pump
            
            // If UV is light up, close it in 5 seconds.
            if (uv = 1) {
                for (float i = 0.99f; i >=0.0f; i-=0.01f) {
                    uv = i;
                    wait(0.02);
                }
            }
        } else {
            // Set up the pump
            // pump = 1;
            // If UV is close, set it up in 5 seconds.
            
            //Check the water quality
            if (waterQuality >= 0.5f) {
                // Check the water level, light up the first RGB.
                if (0.15f < waterLevel <= 0.24f) {
                    myRGBled1R = 0;
                    myRGBled1G = 1;
                    myRGBled2R = 0;
                    myRGBled2G = 0;
                    myRGBled3R = 0;
                    myRGBled3G = 0;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                    //Check the water level, light up the second RGB.
                } else if (0.24f < waterLevel <= 0.27f) {
                    myRGBled1R = 0;
                    myRGBled1G = 1;
                    myRGBled2R = 0;
                    myRGBled2G = 1;
                    myRGBled3R = 0;
                    myRGBled3G = 0;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                } else if (0.27 < waterLevel <= 0.33) {
                    myRGBled1R = 0;
                    myRGBled1G = 1;
                    myRGBled2R = 0;
                    myRGBled2G = 1;
                    myRGBled3R = 0;
                    myRGBled3G = 1;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                } else {
                    myRGBled1R = 0;
                    myRGBled1G = 1;
                    myRGBled2R = 0;
                    myRGBled2G = 1;
                    myRGBled3R = 0;
                    myRGBled3G = 1;
                    myRGBled4R = 0;
                    myRGBled4G = 1;
                }
                //Water quality is bad
            } else {
                if (0.15f < waterLevel <= 0.24f) {
                    myRGBled1R = 1;
                    myRGBled1G = 0;
                    myRGBled2R = 0;
                    myRGBled2G = 0;
                    myRGBled3R = 0;
                    myRGBled3G = 0;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                } else if (0.24f < waterLevel <= 0.27f) {
                    myRGBled1R = 1;
                    myRGBled1G = 0;
                    myRGBled2R = 1;
                    myRGBled2G = 0;
                    myRGBled3R = 0;
                    myRGBled3G = 0;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                } else if (0.27 < waterLevel <= 0.33) {
                    myRGBled1R = 1;
                    myRGBled1G = 0;
                    myRGBled2R = 1;
                    myRGBled2G = 0;
                    myRGBled3R = 1;
                    myRGBled3G = 0;
                    myRGBled4R = 0;
                    myRGBled4G = 0;
                } else {
                    myRGBled1R = 1;
                    myRGBled1G = 0;
                    myRGBled2R = 1;
                    myRGBled2G = 0;
                    myRGBled3R = 1;
                    myRGBled3G = 0;
                    myRGBled4R = 1;
                    myRGBled4G = 0;
                }
            }
        }
        
    }
}
 
// Reads and processes GET and POST web data
void ReadWebData()
{
    wait_ms(200);
    esp.attach(NULL,Serial::RxIrq);
    DataRX=0;
    memset(webdata, '\0', sizeof(webdata));
    strcpy(webdata, rx_buffer);
    memset(rx_buffer, '\0', sizeof(rx_buffer));
    rx_in = 0;
    rx_out = 0;
    // check web data for form information
    if( strstr(webdata, "check=uvv") != NULL ) {
        if (uv < 0.05) {
            for (float i = 0.01f; i < 1.0f; i+=0.01f) {
                uv = i;
                wait(0.02);
            }
        } else if (uv > 0.95) {
            for (float i = 0.99f; i >= 0.0f; i-=0.01f) {
                uv = i;
                wait(0.02);
            } 
        }
    }
    if( strstr(webdata, "check=pumpv") != NULL ) {
        if (pump < 0.05) {
            pump = 1.0;
        } else if (pump > 0.95) {
            pump = 0.0;   
        }
    }
    if( strstr(webdata, "POST") != NULL ) { // set update flag if POST request
        update=1;
    }
    if( strstr(webdata, "GET") != NULL && strstr(webdata, "favicon") == NULL ) { // set update flag for GET request but do not want to update for favicon requests
        update=1;
    }
}
// Starts webserver
void startserver()
{
    gettime();
    getlevel();
    getquality();
    pc.printf("++++++++++ Resetting ESP ++++++++++\r\n");
    strcpy(cmdbuff,"node.restart()\r\n");
    SendCMD();
    wait(2);
    getreply();
    
    pc.printf("\n++++++++++ Starting Server ++++++++++\r\n> ");
 
    // initial values
    sprintf(cmdbuff, "count,time,analog1,analog2=0,\"%s\",\"%s\",\"%s\"\r\n",timebuf,level,quality);
    SendCMD();
    getreply();
    wait(0.5);
 
    //create server
    sprintf(cmdbuff, "srv=net.createServer(net.TCP,%d)\r\n",SERVtimeout);
    SendCMD();
    getreply();
    wait(0.5);
    strcpy(cmdbuff,"srv:listen(80,function(conn)\r\n");
    SendCMD();
    getreply();
    wait(0.3);
        strcpy(cmdbuff,"conn:on(\"receive\",function(conn,payload) \r\n");
        SendCMD();
        getreply();
        wait(0.3);
        
        //print data to mbed
        strcpy(cmdbuff,"print(payload)\r\n");
        SendCMD();
        getreply();
        wait(0.2);
       
        //web page data
        strcpy(cmdbuff,"conn:send('<!DOCTYPE html><html><body><h1>Cat Drinking Fountain</h1>')\r\n");
        SendCMD();
        getreply();
        wait(0.4);
        strcpy(cmdbuff,"conn:send('Submit count: '..count..'')\r\n");
        SendCMD();
        getreply();
        wait(0.2);
        strcpy(cmdbuff,"conn:send('<br>Last submit (based on mbed RTC time): '..time..'<br><hr>')\r\n");
        SendCMD();
        getreply();
        wait(0.4);
        strcpy(cmdbuff,"conn:send('Water quality: '..analog1..' V<br>Water Level: '..analog2..' V<br><hr>')\r\n");
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff,"conn:send('<form method=\"POST\"')\r\n");
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff, "conn:send('<p><input type=\"checkbox\" name=\"check\" value=\"uvv\"> open/close uv')\r\n");
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff, "conn:send('<p><input type=\"checkbox\" name=\"check\" value=\"pumpv\"> open/close pump')\r\n");
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff,"conn:send('<p><input type=\"submit\" value=\"send-refresh\"></form>')\r\n");
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff, "conn:send('<p><h2>How to use:</h2><ul><li>Select a checkbox to flip on/off</li><li>Click Send-Refresh to send data and refresh values</li></ul></body></html>')\r\n");
        SendCMD();
        getreply();
        wait(0.5); 
        // end web page data
        strcpy(cmdbuff, "conn:on(\"sent\",function(conn) conn:close() end)\r\n"); // close current connection
        SendCMD();
        getreply();
        wait(0.3);
        strcpy(cmdbuff, "end)\r\n");
        SendCMD();
        getreply();
        wait(0.2);
    strcpy(cmdbuff, "end)\r\n");
    SendCMD();
    getreply();
    wait(0.2);
 
    strcpy(cmdbuff, "tmr.alarm(0, 1000, 1, function()\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff, "if wifi.sta.getip() == nil then\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff, "print(\"Connecting to AP...\\n\")\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff, "else\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff, "ip, nm, gw=wifi.sta.getip()\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff,"print(\"IP Address: \",ip)\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff,"tmr.stop(0)\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff,"end\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    strcpy(cmdbuff,"end)\r\n");
    SendCMD();
    getreply();
    wait(0.2);
    
    pc.printf("\n\n++++++++++ Ready ++++++++++\r\n\n");
}
 
 
// ESP Command data send
void SendCMD()
{
    int i;
    char temp_char;
    bool empty;
    i = 0;
// Start Critical Section - don't interrupt while changing global buffer variables
    NVIC_DisableIRQ(UART1_IRQn);
    empty = (tx_in == tx_out);
    while ((i==0) || (cmdbuff[i-1] != '\n')) {
// Wait if buffer full
        if (((tx_in + 1) % buffer_size) == tx_out) {
// End Critical Section - need to let interrupt routine empty buffer by sending
            NVIC_EnableIRQ(UART1_IRQn);
            while (((tx_in + 1) % buffer_size) == tx_out) {
            }
// Start Critical Section - don't interrupt while changing global buffer variables
            NVIC_DisableIRQ(UART1_IRQn);
        }
        tx_buffer[tx_in] = cmdbuff[i];
        i++;
        tx_in = (tx_in + 1) % buffer_size;
    }
    if (esp.writeable() && (empty)) {
        temp_char = tx_buffer[tx_out];
        tx_out = (tx_out + 1) % buffer_size;
// Send first character to start tx interrupts, if stopped
        esp.putc(temp_char);
    }
// End Critical Section
    NVIC_EnableIRQ(UART1_IRQn);
    return;
}
 
// Get Command and ESP status replies
void getreply()
{
    read_line();
    sscanf(rx_line,replybuff);
}
 
// Read a line from the large rx buffer from rx interrupt routine
void read_line() {
    int i;
    i = 0;
// Start Critical Section - don't interrupt while changing global buffer variables
    NVIC_DisableIRQ(UART1_IRQn);
// Loop reading rx buffer characters until end of line character
    while ((i==0) || (rx_line[i-1] != '\r')) {
// Wait if buffer empty
        if (rx_in == rx_out) {
// End Critical Section - need to allow rx interrupt to get new characters for buffer
            NVIC_EnableIRQ(UART1_IRQn);
            while (rx_in == rx_out) {
            }
// Start Critical Section - don't interrupt while changing global buffer variables
            NVIC_DisableIRQ(UART1_IRQn);
        }
        rx_line[i] = rx_buffer[rx_out];
        i++;
        rx_out = (rx_out + 1) % buffer_size;
    }
// End Critical Section
    NVIC_EnableIRQ(UART1_IRQn);
    rx_line[i-1] = 0;
    return;
}
 
 
// Interupt Routine to read in data from serial port
void Rx_interrupt() {
    DataRX=1;
    //led3=1;
// Loop just in case more than one character is in UART's receive FIFO buffer
// Stop if buffer full
    while ((esp.readable()) && (((rx_in + 1) % buffer_size) != rx_out)) {
        rx_buffer[rx_in] = esp.getc();
// Uncomment to Echo to USB serial to watch data flow
        pc.putc(rx_buffer[rx_in]);
        rx_in = (rx_in + 1) % buffer_size;
    }
    //led3=0;
    return;
}
 
 
// Interupt Routine to write out data to serial port
void Tx_interrupt() {
    //led2=1;
// Loop to fill more than one character in UART's transmit FIFO buffer
// Stop if buffer empty
    while ((esp.writeable()) && (tx_in != tx_out)) {
        esp.putc(tx_buffer[tx_out]);
        tx_out = (tx_out + 1) % buffer_size;
    }
    //led2=0;
    return;
}
 
void gettime()
{
    time_t seconds = time(NULL);
    strftime(timebuf,50,"%H:%M:%S %a %d %b %y", localtime(&seconds));
}
 
void setRTC()
{
    t.tm_sec = (0);             // 0-59
    t.tm_min = (minute);        // 0-59
    t.tm_hour = (hour);         // 0-23
    t.tm_mday = (dayofmonth);   // 1-31
    t.tm_mon = (month-1);       // 0-11  "0" = Jan, -1 added for Mbed RCT clock format
    t.tm_year = ((year)+100);   // year since 1900,  current DCF year + 100 + 1900 = correct year
    set_time(mktime(&t));       // set RTC clock
}
// Analog in example
void getquality()
{
    AdcIn=waterQuality.read();
    Ht = (AdcIn*3.3); // set the numeric to the exact MCU analog reference voltage for greater accuracy
    sprintf(quality,"%2.3f",Ht);
}
// Temperature example
void getlevel()
{
 
    AdcIn=waterLevel.read();
    Ht = (AdcIn*3.3); // set the numeric to the exact MCU analog reference voltage for greater accuracy  
    sprintf(level,"%2.3f",Ht);
}