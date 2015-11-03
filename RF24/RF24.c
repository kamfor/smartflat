#define TX_PORT		PORTA
#define TX_PORT_PIN	PINA
#define TX_PORT_DD	DDRA

#define TX_CE	1 //Output
#define TX_CSN	3 //Output
#define TX_SCK	5 //Output
#define TX_MOSI	6 //Output
#define TX_MISO 4 //Input
//#define TX_IRQ	0 //Input


#define RX_PORT 	PORTB
#define RX_PORT_PIN PINB
#define RX_PORT_DD	DDRB

#define RX_CE	1 //Output
#define RX_CSN  2 //Output
#define RX_SCK  5 //Output
#define RX_MOSI 3 //Output
#define RX_MISO	4 //Input
//#define RX_IRQ	3 //Input

#define RF_DELAY	5

uint8_t data_array[4];


//2.4G Configuration - Transmitter
uint8_t configure_transmitter(void);
//Sends command to nRF
uint8_t tx_send_byte(uint8_t cmd);
//Basic SPI to nRF
uint8_t tx_send_command(uint8_t cmd, uint8_t data);
//Sends the 4 bytes of payload
void tx_send_payload(uint8_t cmd);
//This sends out the data stored in the data_array
void transmit_data(void);
//Basic SPI to nRF
uint8_t tx_spi_byte(uint8_t outgoing);


//2.4G Configuration - Receiver
void configure_receiver(void);
//Sends one byte to nRF
uint8_t rx_send_byte(uint8_t cmd);
//Sends command to nRF
uint8_t rx_send_command(uint8_t cmd, uint8_t data);
//Sends the 4 bytes of payload
void rx_send_payload(uint8_t cmd);
//Basic SPI to nRF
uint8_t rx_spi_byte(uint8_t outgoing);


void init_nRF_pins(void)
{
	//1 = Output, 0 = Input
	//TX_PORT_DD = 0xFF & ~(1<<TX_MISO);// | 1<<TX_IRQ);
	RX_PORT_DD = 0xFF & ~(1<<RX_MISO);// | 1<<RX_IRQ);

	//Enable pull-up resistors (page 74)
	//TX_PORT = 0b11111111;
	RX_PORT = 0b11111111;

	cbi(RX_PORT, RX_CE); //Stand by mode
	//cbi(TX_PORT, TX_CE); //Stand by mode
}

//RX Functions
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

/*
//This will clock out the current payload into the data_array
void receive_data(void)
{
    uint8_t i, j, temp;
	
	temp = 0;

    cbi(RX_PORT, RX_CE); //RX_CE = 0;//Power down RF Front end
    //delay_us(RF_DELAY);

    //Clock in data, we are setup for 32-bit payloads
    for(i = 0 ; i < 4 ; i++) //4 bytes
    {
        for(j = 0 ; j < 8 ; j++) //8 bits each
        {
            temp <<= 1;
			if(RX_PORT_PIN & (1<<RX_DATA)) temp |= 1;

			sbi(RX_PORT, RX_CLK);
			//delay_us(RF_DELAY);
			cbi(RX_PORT, RX_CLK);
        }

        data_array[i] = temp; //Store this byte
    }
    
	//if (data_array[0] == 0x12 && data_array[1] == 0x34) good_packets++;

    sbi(RX_PORT, RX_CE); //RX_CE = 1; //Power up RF Front end
}
*/



//Reads the current RX buffer into the data array
//Forces an RX buffer flush
void receive_data(void)
{
	cbi(RX_PORT, RX_CSN); //Stand by mode
    rx_spi_byte(0x61); //Read RX Payload
	data_array[0] = rx_spi_byte(0xFF);
	data_array[1] = rx_spi_byte(0xFF);
	data_array[2] = rx_spi_byte(0xFF);
	data_array[3] = rx_spi_byte(0xFF);
	sbi(RX_PORT, RX_CSN);
	
	rx_send_byte(0xE2); //Flush RX FIFO
	
	rx_send_command(0x27, 0x40); //Clear RF FIFO interrupt

    sbi(RX_PORT, RX_CE); //Go back to receiving!
}

//2.4G Configuration - Receiver
//This setups up a RF-24G for receiving at 1mbps
void configure_receiver(void)
{
    cbi(RX_PORT, RX_CE); //Go into standby mode

	rx_send_command(0x20, 0x39); //Enable RX IRQ, CRC Enabled, be a receiver

	rx_send_command(0x21, 0x00); //Disable auto-acknowledge

	rx_send_command(0x23, 0x03); //Set address width to 5bytes (default, not really needed)

	rx_send_command(0x26, 0x07); //Air data rate 1Mbit, 0dBm, Setup LNA

	rx_send_command(0x31, 0x04); //4 byte receive payload

	rx_send_command(0x25, 0x02); //RF Channel 2 (default, not really needed)

	data_array[0] = 0xE7;
	data_array[1] = 0xE7;
	data_array[2] = 0xE7;
	data_array[3] = 0xE7;
	rx_send_payload(0x2A); //Set RX pipe 0 address

	rx_send_command(0x20, 0x3B); //RX interrupt, power up, be a receiver

    sbi(RX_PORT, RX_CE); //Start receiving!
}    

//Sends the 4 bytes of payload
void rx_send_payload(uint8_t cmd)
{
	uint8_t i;

	cbi(RX_PORT, RX_CSN); //Select chip
	rx_spi_byte(cmd);
	
	for(i = 0 ; i < 4 ; i++)
		rx_spi_byte(data_array[i]);

	sbi(RX_PORT, RX_CSN); //Deselect chip
}

//Sends command to nRF
uint8_t rx_send_command(uint8_t cmd, uint8_t data)
{
	uint8_t status;

	cbi(RX_PORT, RX_CE); //Stand by mode

	cbi(RX_PORT, RX_CSN); //Select chip
	rx_spi_byte(cmd);
	status = rx_spi_byte(data);
	sbi(RX_PORT, RX_CSN); //Deselect chip
	
	return(status);
}

//Sends one byte to nRF
uint8_t rx_send_byte(uint8_t cmd)
{
	uint8_t status;

	cbi(RX_PORT, RX_CSN); //Select chip
	status = rx_spi_byte(cmd);
	sbi(RX_PORT, RX_CSN); //Deselect chip
	
	return(status);
}

//Basic SPI to nRF
uint8_t rx_spi_byte(uint8_t outgoing)
{
    uint8_t i, incoming;
	incoming = 0;

    //Send outgoing byte
    for(i = 0 ; i < 8 ; i++)
    {

		if(outgoing & 0b10000000)
			sbi(RX_PORT, RX_MOSI);
		else
			cbi(RX_PORT, RX_MOSI);

		sbi(RX_PORT, RX_SCK); //RX_SCK = 1;
		delay_us(RF_DELAY);

		incoming <<= 1;
		if( RX_PORT_PIN & (1<<RX_MISO) ) incoming |= 0x01;
		
		cbi(RX_PORT, RX_SCK); //RX_SCK = 0; 
		delay_us(RF_DELAY);

        outgoing <<= 1;

    }

	return(incoming);
}

//TX Functions
//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=



//This sends out the data stored in the data_array
//data_array must be setup before calling this function
void transmit_data(void)
{
	tx_send_command(0x27, 0x7E); //Clear any interrupts
	
	tx_send_command(0x20, 0x7A); //Power up and be a transmitter

	tx_send_byte(0xE1); //Clear TX Fifo
	
	data_array[0] = 0xA1;
	data_array[1] = 0xA1;
	data_array[2] = 0xA1;
	data_array[3] = 0xA0;
	tx_send_payload(0xA0); //Clock in 4 byte payload of data_array

    sbi(TX_PORT, TX_CE); //Pulse CE to start transmission
    delay_ms(1);
    cbi(TX_PORT, TX_CE);
}

//2.4G Configuration - Transmitter
//This sets up one RF-24G for shockburst transmission
uint8_t configure_transmitter(void)
{
    cbi(TX_PORT, TX_CE); //Go into standby mode
	
	tx_send_command(0x20, 0x78); //CRC enabled, be a transmitter

	tx_send_command(0x21, 0x00); //Disable auto acknowledge on all pipes

	tx_send_command(0x24, 0x00); //Disable auto-retransmit

	tx_send_command(0x23, 0x03); //Set address width to 5bytes (default, not really needed)

	tx_send_command(0x26, 0x07); //Air data rate 1Mbit, 0dBm, Setup LNA

	tx_send_command(0x25, 0x02); //RF Channel 2 (default, not really needed)

	data_array[0] = 0xE7;
	data_array[1] = 0xE7;
	data_array[2] = 0xE7;
	data_array[3] = 0xE7;
	tx_send_payload(0x30); //Set TX address
	
	tx_send_command(0x20, 0x7A); //Power up, be a transmitter

	return(tx_send_byte(0xFF));
}

//Sends the 4 bytes of payload
void tx_send_payload(uint8_t cmd)
{
	uint8_t i;

	cbi(TX_PORT, TX_CSN); //Select chip
	tx_spi_byte(cmd);
	
	for(i = 0 ; i < 4 ; i++)
		tx_spi_byte(data_array[i]);

	sbi(TX_PORT, TX_CSN); //Deselect chip
}

//Sends command to nRF
uint8_t tx_send_command(uint8_t cmd, uint8_t data)
{
	uint8_t status;

	cbi(TX_PORT, TX_CSN); //Select chip
	tx_spi_byte(cmd);
	status = tx_spi_byte(data);
	sbi(TX_PORT, TX_CSN); //Deselect chip

	return(status);
}

//Sends one byte to nRF
uint8_t tx_send_byte(uint8_t cmd)
{
	uint8_t status;
	
	cbi(TX_PORT, TX_CSN); //Select chip
	status = tx_spi_byte(cmd);
	sbi(TX_PORT, TX_CSN); //Deselect chip
	
	return(status);
}

//Basic SPI to nRF
uint8_t tx_spi_byte(uint8_t outgoing)
{
    uint8_t i, incoming;
	incoming = 0;

    //Send outgoing byte
    for(i = 0 ; i < 8 ; i++)
    {
		if(outgoing & 0b10000000)
			sbi(TX_PORT, TX_MOSI);
		else
			cbi(TX_PORT, TX_MOSI);
		
		sbi(TX_PORT, TX_SCK); //TX_SCK = 1;
		delay_us(RF_DELAY);

		//MISO bit is valid after clock goes going high
		incoming <<= 1;
		if( TX_PORT_PIN & (1<<TX_MISO) ) incoming |= 0x01;

		cbi(TX_PORT, TX_SCK); //TX_SCK = 0; 
		delay_us(RF_DELAY);
		
        outgoing <<= 1;
    }

	return(incoming);
}
