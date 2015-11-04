#define PORT		PORTA
#define PORT_PIN	PINA
#define PORT_DD		DDRA

#define CE	1 //Output
#define CSN	3 //Output
#define SCK	5 //Output
#define MOSI	6 //Output
#define MISO	4 //Input
//#define IRQ	0 //Input

#define RF_DELAY	5

uint8_t data_array[4];

//2.4G Configuration - Transmitter
uint8_t configure_transmitter(void);
//Sends command to nRF
uint8_t send_byte(uint8_t cmd);
//Basic SPI to nRF
uint8_t tx_send_command(uint8_t cmd, uint8_t data);
//Sends the 4 bytes of payload
void send_payload(uint8_t cmd);
//This sends out the data stored in the data_array
void transmit_data(void);
//Basic SPI to nRF
uint8_t spi_byte(uint8_t outgoing);
//2.4G Configuration - Receiver
void configure_receiver(void);
//Sends command to nRF
uint8_t rx_send_command(uint8_t cmd, uint8_t data);


void init_nRF_pins(void)
{
	//1 = Output, 0 = Input
	PORT_DD = 0xFF & ~(1<<RX_MISO);// | 1<<RX_IRQ);

	//Enable pull-up resistors (page 74)
	PORT = 0b11111111;

	cbi(PORT, CE); //Stand by mode
}

//Reads the current RX buffer into the data array
//Forces an RX buffer flush
void receive_data(void)
{
	cbi(PORT, CSN); //Stand by mode
  	spi_byte(0x61); //Read RX Payload
	data_array[0] = spi_byte(0xFF);
	data_array[1] = spi_byte(0xFF);
	data_array[2] = spi_byte(0xFF);
	data_array[3] = spi_byte(0xFF);
	sbi(PORT, CSN);
	
	send_byte(0xE2); //Flush RX FIFO
	
	rx_send_command(0x27, 0x40); //Clear RF FIFO interrupt

	sbi(PORT, CE); //Go back to receiving!
}

//This sends out the data stored in the data_array
//data_array must be setup before calling this function
void transmit_data(void)
{
	tx_send_command(0x27, 0x7E); //Clear any interrupts
	
	tx_send_command(0x20, 0x7A); //Power up and be a transmitter

	send_byte(0xE1); //Clear TX Fifo
	
	data_array[0] = 0xA1;
	data_array[1] = 0xA1;
	data_array[2] = 0xA1;
	data_array[3] = 0xA0;
	send_payload(0xA0); //Clock in 4 byte payload of data_array

	sbi(PORT, CE); //Pulse CE to start transmission
	delay_ms(1);
	cbi(PORT, CE);
}

//2.4G Configuration - Receiver
//This setups up a RF-24G for receiving at 1mbps
void configure_receiver(void)
{
	cbi(PORT, CE); //Go into standby mode

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
	send_payload(0x2A); //Set RX pipe 0 address

	rx_send_command(0x20, 0x3B); //RX interrupt, power up, be a receiver

	sbi(PORT, CE); //Start receiving!
}    

//2.4G Configuration - Transmitter
//This sets up one RF-24G for shockburst transmission
uint8_t configure_transmitter(void)
{
	cbi(PORT, CE); //Go into standby mode
	
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
	send_payload(0x30); //Set TX address
	
	tx_send_command(0x20, 0x7A); //Power up, be a transmitter

	return(send_byte(0xFF));
}


//Sends the 4 bytes of payload
void send_payload(uint8_t cmd)
{
	uint8_t i;

	cbi(PORT, CSN); //Select chip
	spi_byte(cmd);
	
	for(i = 0 ; i < 4 ; i++)
		spi_byte(data_array[i]);

	sbi(PORT, CSN); //Deselect chip
}


//Sends command to nRF
uint8_t rx_send_command(uint8_t cmd, uint8_t data)
{
	uint8_t status;

	cbi(PORT, CE); //Stand by mode

	cbi(PORT, CSN); //Select chip
	spi_byte(cmd);
	status = spi_byte(data);
	sbi(PORT, CSN); //Deselect chip
	
	return(status);
}

//Sends command to nRF
uint8_t tx_send_command(uint8_t cmd, uint8_t data)
{
	uint8_t status;

	cbi(PORT, CSN); //Select chip
	spi_byte(cmd);
	status = spi_byte(data);
	sbi(PORT, CSN); //Deselect chip

	return(status);
}

//Sends one byte to nRF
uint8_t send_byte(uint8_t cmd)
{
	uint8_t status;
	
	cbi(PORT, CSN); //Select chip
	status = spi_byte(cmd);
	sbi(PORT, CSN); //Deselect chip
	
	return(status);
}

//Basic SPI to nRF
uint8_t spi_byte(uint8_t outgoing)
{
    uint8_t i, incoming;
	incoming = 0;

    //Send outgoing byte
    for(i = 0 ; i < 8 ; i++)
    {
		if(outgoing & 0b10000000)
			sbi(PORT, MOSI);
		else
			cbi(PORT, MOSI);
		
		sbi(PORT, SCK); //SCK = 1;
		delay_us(RF_DELAY);

		//MISO bit is valid after clock goes going high
		incoming <<= 1;
		if( PORT_PIN & (1<<MISO) ) incoming |= 0x01;

		cbi(PORT, SCK); //SCK = 0; 
		delay_us(RF_DELAY);
		
		outgoing <<= 1;
    }

	return(incoming);
}
