/*
*********************************************************************************************************
*
*	ģ������ : DM9000����
*	�ļ����� : DM9000_BSP.c
*	��    �� : V1.0
*	˵    �� : DM9000�ײ�����ʵ��
*
*	�޸ļ�¼ :
*		�汾��    ����        ����     ˵��
*		V1.0    2020-11-21   Eric2013 ��ʽ����
*
*	Copyright (C), 2020-2030, ���������� www.armfly.com
*
*********************************************************************************************************
*/
#include "bsp.h"
#include "DM9000_BSP.h"
#include "stdio.h"



/*
*********************************************************************************************************
*	                               �궨��
*********************************************************************************************************
*/
#define printk(...)
//#define printk printf

#define Fix_Note_Address
//#define DM9000A_FLOW_CONTROL
//#define DM9000A_UPTO_100M
//#define Fifo_Point_Check
//#define Point_Error_Reset
//#define FifoPointCheck

/* DM9000A ���պ������ú� */
#define Rx_Int_enable
#define Max_Int_Count			1
#define Max_Ethernet_Lenth		1536
#define Broadcast_Jump
#define Max_Broadcast_Lenth		500

/* DM9000A ���ͺ������ú� */
#define Max_Send_Pack			2

#define   NET_BASE_ADDR		0x68400000
#define   NET_REG_ADDR		(*((volatile uint16_t *) NET_BASE_ADDR))
#define   NET_REG_DATA		(*((volatile uint16_t *) (NET_BASE_ADDR + 0x00080000)))

/* DM9000�жϴ��� */
#define BUSY_RCC_GPIO_CLK_ENABLE    __HAL_RCC_GPIOA_CLK_ENABLE
#define BUSY_GPIO		            GPIOA
#define BUSY_PIN		            GPIO_PIN_15
#define BUSY_IRQn		            EXTI15_10_IRQn
#define BUSY_IRQHandler	            EXTI15_10_IRQHandler


/*
*********************************************************************************************************
*	                                     ����
*********************************************************************************************************
*/
static uint8_t SendPackOk = 0;
static uint8_t s_FSMC_Init_Ok = 0;	/* ����ָʾFSMC�Ƿ��ʼ�� */

static void dm9k_debug_test(void);
static void dm9k_udelay(uint16_t time);

static void DM9K_CtrlLinesConfig(void);
static void DM9K_FSMCConfig(void);


/*
*********************************************************************************************************
*	������: dm9k_udelay
*	��  ��: time �� �ӳ�ʱ�䣬����ȷ��us����
*	��  ��: ��
*	��  ��: �ӳٺ���
*********************************************************************************************************
*/
void dm9k_udelay(uint16_t time)
{
    //bsp_DelayUS(time);
    while(time--)
    {
        for(int i = 0;i < 200; i++)
        {
            __NOP();
        }
    }
}

/*
*********************************************************************************************************
*	������: ior
*	��  ��: reg ���Ĵ�����ַ
*	��  ��: ��
*	��  ��: �����Ĵ�����ֵ
*********************************************************************************************************
*/
uint8_t ior(uint8_t reg)
{
	NET_REG_ADDR = reg;
	return (NET_REG_DATA);
}

/*
*********************************************************************************************************
*	������: iow
*	��  ��: reg ���Ĵ�����ַ
*			writedata : д�������
*	��  ��: ��
*	��  ��: дDM9000AE�Ĵ�����ֵ
*********************************************************************************************************
*/
void iow(uint8_t reg, uint8_t writedata)
{
	NET_REG_ADDR = reg;
	NET_REG_DATA = writedata;
}

/*
*********************************************************************************************************
*	������: dm9k_hash_table
*	��  ��: ��
*	��  ��: ��
*	��  ��: ���� DM9000A MAC �� �㲥 �� �ಥ �Ĵ���
*********************************************************************************************************
*/
void dm9k_hash_table(void)
{
	uint8_t i;

	for(i = 0; i < 8; i++) 					/* ��� �����ಥ���� */
    {
        iow(DM9000_REG_MAR + i, 0x00);
    }
		
	iow(DM9000_REG_MAR + 7, 0x80);  		/* ������ �㲥�� ���� */
}

/*
*********************************************************************************************************
*	������: dm9k_reset
*	��  ��: ��
*	��  ��: ��
*	��  ��: ��DM9000AE����������λ
*********************************************************************************************************
*/
void dm9k_reset(void)
{
	iow(DM9000_REG_NCR, DM9000_REG_RESET);			/* �� DM9000A ������������ */
	dm9k_udelay(10);								/* delay 10us */
	iow(DM9000_REG_NCR, DM9000_REG_RESET);			/* �� DM9000A ������������ */
	dm9k_udelay(10);								/* delay 10us */

	/* �����Ǵ���������� */
	iow(DM9000_REG_IMR, DM9000_IMR_OFF); 			/* �����ڴ��Ի�ģʽ */
	iow(DM9000_REG_TCR2, DM9000_TCR2_SET);			/* ���� LED ��ʾģʽ1:ȫ˫��������˫���� */

	/* ���������Ѷ */
	iow(DM9000_REG_NSR, 0x2c);
	iow(DM9000_REG_TCR, 0x00);
	iow(DM9000_REG_ISR, 0x3f);

#ifdef DM9000A_FLOW_CONTROL
	iow(DM9000_REG_BPTR, DM9000_BPTR_SET);			/* ��˫���������� */
	iow(DM9000_REG_FCTR, DM9000_FCTR_SET);			/* ȫ˫���������� */
	iow(DM9000_REG_FCR, DM9000_FCR_SET);			/* ������������ */
#endif

#ifdef DM9000A_UPTO_100M
	/* DM9000A�޴˼Ĵ��� */
	iow(DM9000_REG_OTCR, DM9000_OTCR_SET);			/* ����Ƶ�ʵ� 100Mhz ���� */
#endif

#ifdef  Rx_Int_enable
	iow(DM9000_REG_IMR, DM9000_IMR_SET);			/* ���� �ж�ģʽ */
#else
	iow(DM9000_REG_IMR, DM9000_IMR_OFF);			/* �ر� �ж�ģʽ */
#endif

	iow(DM9000_REG_RCR, DM9000_RCR_SET);			/* ���� ���չ��� */

	SendPackOk = 0;
}

/*
*********************************************************************************************************
*	������: dm9k_phy_write
*	��  ��: phy_reg �� �Ĵ�����ַ
*			writedata �� д�������
*	��  ��: ��
*	��  ��: дDM9000A PHY �Ĵ���
*********************************************************************************************************
*/
void dm9k_phy_write(uint8_t phy_reg, uint16_t writedata)
{
	/* ����д�� PHY �Ĵ�����λ�� */
	iow(DM9000_REG_EPAR, phy_reg | DM9000_PHY);

	/* ����д�� PHY �Ĵ�����ֵ */
	iow(DM9000_REG_EPDRH, ( writedata >> 8 ) & 0xff);
	iow(DM9000_REG_EPDRL, writedata & 0xff);

	iow(DM9000_REG_EPCR, 0x0a); 						/* ������д�� PHY �Ĵ��� */
	while(ior(DM9000_REG_EPCR) & 0x01);					/* ��Ѱ�Ƿ�ִ�н��� */
	iow(DM9000_REG_EPCR, 0x00); 						/* ���д������ */
}

/*
*********************************************************************************************************
*	������: dm9k_phy_read
*	��  ��: phy_reg �� �Ĵ�����ַ
*	��  ��: ��
*	��  ��: ��DM9000A PHY �Ĵ���
*********************************************************************************************************
*/
uint16_t dm9k_phy_read(uint8_t phy_reg)
{
	uint16_t readdata;

	/* ����д�� PHY �Ĵ�����λ�� */
	iow(DM9000_REG_EPAR, phy_reg | DM9000_PHY);

	/* ����д�� PHY �Ĵ�����ֵ */
	iow(DM9000_REG_EPCR, 0x0C);						/* ������д�� PHY �Ĵ��� */
	
	while(ior(DM9000_REG_EPCR) & 0x01);				/* ��Ѱ�Ƿ�ִ�н��� */

	iow(DM9000_REG_EPCR, 0x00);						/* �����ȡ���� */	
	
	readdata = (ior(DM9000_REG_EPDRH) << 8) | (ior(DM9000_REG_EPDRL));
	
	return readdata;
}

/*
*********************************************************************************************************
*	������: dm9k_initnic
*	��  ��: ��
*	��  ��: ��
*	��  ��: ��ʼ��DM9000AE
*********************************************************************************************************
*/
void dm9k_initnic(void)
{
	iow(DM9000_REG_NCR, DM9000_REG_RESET);			/* �� DM9000A ������������ */
	dm9k_udelay(10);								/* delay 10us */

	dm9k_hash_table();								/* ���� DM9000A MAC �� �ಥ*/

	dm9k_reset();									/* ���� DM9000A �������� */

	iow(DM9000_REG_GPR, DM9000_PHY_OFF);			/* �ر� PHY ������ PHY ����*/
	dm9k_phy_write(0x00, 0x8000);					/* ���� PHY �ļĴ��� */
#ifdef DM9000A_FLOW_CONTROL
	dm9k_phy_write(0x04, 0x01e1 | 0x0400);			/* ���� ����Ӧģʽ���ݱ� */
#else
	dm9k_phy_write(0x04, 0x01e1);					/* ���� ����Ӧģʽ���ݱ� */
#endif
	/* ����ģʽ����
	  0x0000 : �̶�10M��˫��
	  0x0100 : �̶�10Mȫ˫��
	  0x2000 : �̶�100M��˫��
	  0x2100 : �̶�100Mȫ˫��
	  0x1000 : ����Ӧģʽ
	*/
	dm9k_phy_write(0x00, 0x1000);				  /* ���� ��������ģʽ */

	iow(DM9000_REG_GPR, DM9000_PHY_ON);			  /* ���� PHY ����, ���� PHY */
}

/*
*********************************************************************************************************
*	������: GetSize
*	��  ��: ��
*	��  ��: ���յ������ݴ�С
*	��  ��: ��ȡ���յ������ݴ�С
*********************************************************************************************************
*/
uint16_t GetSize(void)
{
	uint16_t ReceiveLength;
	uint8_t  rx_int_count = 0;
	uint8_t  rx_checkbyte;
	uint16_t rx_status, rx_length;
	uint8_t  jump_packet;
	uint16_t calc_len;
	uint16_t calc_MRR;

	do
	{
		ReceiveLength = 0;								/* ������յĳ��� */
		jump_packet = 0;								/* ����������� */
		ior(DM9000_REG_MRCMDX);							/* ��ȡ�ڴ����ݣ���ַ������ */
		/* �����ڴ�����λ�� */
		calc_MRR = (ior(DM9000_REG_MRRH) << 8) + ior(DM9000_REG_MRRL);
		rx_checkbyte = ior(DM9000_REG_MRCMDX);			/*  */

		if(rx_checkbyte == DM9000_PKT_RDY)				/* ȡ */
		{
			/* ��ȡ��������Ѷ �� ���� */
			NET_REG_ADDR = DM9000_REG_MRCMD;
			rx_status = NET_REG_DATA;
			rx_length = NET_REG_DATA;

			/* ���յ�����ϵͳ�ɳ��ܵķ�����˰����� */
			if(rx_length > Max_Ethernet_Lenth)
				jump_packet = 1;

#ifdef Broadcast_Jump
			/* ���յ��Ĺ㲥��ಥ�������ض����ȣ��˰����� */
			if(rx_status & 0x4000)
			{
				if(rx_length > Max_Broadcast_Lenth)
					jump_packet = 1;
			}
#endif
			/* ������һ������ָ��λ , �����ճ���Ϊ���������һ����ż�ֽڡ�*/
			/* ���ǳ��� 0x3fff ������ع��Ƶ� 0x0c00 ��ʼλ�� */
			calc_MRR += (rx_length + 4);
			if(rx_length & 0x0001) calc_MRR++;
			if(calc_MRR > 0x3fff) calc_MRR -= 0x3400;

			if(jump_packet == 0x01)
			{
				/* ��ָ���Ƶ���һ�����İ�ͷλ�� */
				iow (DM9000_REG_MRRH, (calc_MRR >> 8) & 0xff);
				iow (DM9000_REG_MRRL, calc_MRR & 0xff );
				continue;
			}

			/* ��ʼ���ڴ�����ϰᵽ��ϵͳ�У�ÿ���ƶ�һ�� word */
			calc_len = (rx_length + 1) >> 1;
            
            (void)calc_len; 
			
			//for(i = 0 ; i < calc_len ; i++)
			//	ReceiveData[i] = NET_REG_DATA;
			
			/* �������ر��� TCP/IP �ϲ㣬����ȥ���� 4 BYTE �� CRC-32 ����� */
			ReceiveLength = rx_length - 4;

			rx_int_count++;								/* �ۼ��հ����� */

#ifdef FifoPointCheck
			if(calc_MRR != ((ior(DM9000_REG_MRRH) << 8) + ior(DM9000_REG_MRRL)))
			{
#ifdef Point_Error_Reset
				dm9k_reset();								/* ����ָ����������� */
				return ReceiveLength;
#endif
				/*����ָ���������ָ���Ƶ���һ�����İ�ͷλ��  */
				iow(DM9000_REG_MRRH, (calc_MRR >> 8) & 0xff);
				iow(DM9000_REG_MRRL, calc_MRR & 0xff);
			}
#endif
			return ReceiveLength;
		}
		else
		{
			if(rx_checkbyte == DM9000_PKT_NORDY)		/* δ�յ��� */
			{
				iow(DM9000_REG_ISR, 0x3f);				
			}
			else
			{
				dm9k_reset();							/* ����ָ����������� */
			}
			return (0);
		}
	}while(rx_int_count < Max_Int_Count);				/* �Ƿ񳬹������շ������ */
	return 0;
	
}

/*
*********************************************************************************************************
*	������: dm9k_receive_packet
*	��  ��: _uip_buf : ���ջ�����
*	��  ��: > 0 ��ʾ���յ����ݳ���, 0��ʾû������
*	��  ��: ��ȡһ������
*********************************************************************************************************
*/
uint16_t dm9k_receive_packet(uint8_t *_uip_buf)
{
	uint16_t ReceiveLength;
	uint16_t *ReceiveData;
	uint8_t  rx_int_count = 0;
	uint8_t  rx_checkbyte;
	uint16_t rx_status, rx_length;
	uint8_t  jump_packet;
	uint16_t calc_len;
	uint16_t calc_MRR;
	uint32_t blkCnt; 

	do
	{
		ReceiveLength = 0;								/* ������յĳ��� */
		ReceiveData = (uint16_t *)_uip_buf;
		jump_packet = 0;								/* ����������� */
		ior(DM9000_REG_MRCMDX);							/* ��ȡ�ڴ����ݣ���ַ������ */
		/* �����ڴ�����λ�� */
		calc_MRR = (ior(DM9000_REG_MRRH) << 8) + ior(DM9000_REG_MRRL);
		rx_checkbyte = ior(DM9000_REG_MRCMDX);			/*  */

		if(rx_checkbyte == DM9000_PKT_RDY)				/* ȡ */
		{
			/* ��ȡ��������Ѷ �� ���� */
			NET_REG_ADDR = DM9000_REG_MRCMD;
			rx_status = NET_REG_DATA;
			rx_length = NET_REG_DATA;

			/* ���յ�����ϵͳ�ɳ��ܵķ�����˰����� */
			if(rx_length > Max_Ethernet_Lenth)
				jump_packet = 1;

#ifdef Broadcast_Jump
			/* ���յ��Ĺ㲥��ಥ�������ض����ȣ��˰����� */
			if(rx_status & 0x4000)
			{
				if(rx_length > Max_Broadcast_Lenth)
					jump_packet = 1;
			}
#endif
			/* ������һ������ָ��λ , �����ճ���Ϊ���������һ����ż�ֽڡ�*/
			/* ���ǳ��� 0x3fff ������ع��Ƶ� 0x0c00 ��ʼλ�� */
			calc_MRR += (rx_length + 4);
			if(rx_length & 0x0001) calc_MRR++;
			if(calc_MRR > 0x3fff) calc_MRR -= 0x3400;

			if(jump_packet == 0x01)
			{
				/* ��ָ���Ƶ���һ�����İ�ͷλ�� */
				iow (DM9000_REG_MRRH, (calc_MRR >> 8) & 0xff);
				iow (DM9000_REG_MRRL, calc_MRR & 0xff );
				continue;
			}

			/* ��ʼ���ڴ�����ϰᵽ��ϵͳ�У�ÿ���ƶ�һ�� word */
			calc_len = (rx_length + 1) >> 1;
			
			//for(i = 0 ; i < calc_len ; i++)
			//	ReceiveData[i] = NET_REG_DATA;
			
			blkCnt = calc_len >> 2U;
			
			while (blkCnt > 0U)
			{
				*ReceiveData++ = NET_REG_DATA;
				*ReceiveData++ = NET_REG_DATA;
				*ReceiveData++ = NET_REG_DATA;
				*ReceiveData++ = NET_REG_DATA;
				blkCnt--;
			}

			blkCnt = calc_len % 0x4U;

			while (blkCnt > 0U)
			{
				*ReceiveData++ = NET_REG_DATA;
				blkCnt--;
			}
			
			/* �������ر��� TCP/IP �ϲ㣬����ȥ���� 4 BYTE �� CRC-32 ����� */
			ReceiveLength = rx_length - 4;

			rx_int_count++;								/* �ۼ��հ����� */

#ifdef FifoPointCheck
			if(calc_MRR != ((ior(DM9000_REG_MRRH) << 8) + ior(DM9000_REG_MRRL)))
			{
#ifdef Point_Error_Reset
				dm9k_reset();								/* ����ָ����������� */
				return ReceiveLength;
#endif
				/*����ָ���������ָ���Ƶ���һ�����İ�ͷλ��  */
				iow(DM9000_REG_MRRH, (calc_MRR >> 8) & 0xff);
				iow(DM9000_REG_MRRL, calc_MRR & 0xff);
			}
#endif
			return ReceiveLength;
		}
		else
		{
			if(rx_checkbyte == DM9000_PKT_NORDY)		/* δ�յ��� */
			{
				iow(DM9000_REG_ISR, 0x3f);				/*  */
			}
			else
			{
				dm9k_reset();								/* ����ָ����������� */
			}
			return (0);
		}
	}while(rx_int_count < Max_Int_Count);				/* �Ƿ񳬹������շ������ */
	return 0;
}

/*
*********************************************************************************************************
*	������: dm9k_send_packet
*	��  ��: p_char : �������ݻ�����
*			length : ���ݳ���
*	��  ��: ��
*	��  ��: ����һ������
*********************************************************************************************************
*/
void dm9k_send_packet(uint8_t *p_char, uint16_t length)
{
	uint16_t SendLength = length;
	uint16_t *SendData = (uint16_t *) p_char;
	uint16_t calc_len;
	__IO uint16_t calc_MWR;
    uint32_t blkCnt; 

	/* ��� DM9000A �Ƿ��ڴ����У����ǵȴ�ֱ�����ͽ��� */
	if(SendPackOk == Max_Send_Pack)
	{
		while(ior(DM9000_REG_TCR) & DM9000_TCR_SET)
		{
			dm9k_udelay (5);
		}
		SendPackOk = 0;
	}

	SendPackOk++;										/* ���ô��ͼ��� */

#ifdef FifoPointCheck
	/* ������һ�����͵�ָ��λ , �����ճ���Ϊ���������һ����ż�ֽڡ�*/
	/* ���ǳ��� 0x0bff ������ع��Ƶ� 0x0000 ��ʼλ�� */
	calc_MWR = (ior(DM9000_REG_MWRH) << 8) + ior(DM9000_REG_MWRL);
	calc_MWR += SendLength;
	if(SendLength & 0x01) calc_MWR++;
	if(calc_MWR > 0x0bff) calc_MWR -= 0x0c00;
#endif

	iow(DM9000_REG_TXPLH, (SendLength >> 8) & 0xff);	/* ���ô��ͷ���ĳ��� */
	iow(DM9000_REG_TXPLL, SendLength & 0xff);

	/* ��ʼ��ϵͳ�����ϰᵽ���ڴ��У�ÿ���ƶ�һ�� word */
	NET_REG_ADDR = DM9000_REG_MWCMD;
	calc_len = (SendLength + 1) >> 1;
    
	//for(i = 0; i < calc_len; i++)
	//	NET_REG_DATA = SendData[i];
    
    blkCnt = calc_len >> 2U;
			
    while (blkCnt > 0U)
    {
        NET_REG_DATA = *SendData++;
        NET_REG_DATA = *SendData++;
        NET_REG_DATA = *SendData++;
        NET_REG_DATA = *SendData++;
        blkCnt--;
    }

    blkCnt = calc_len % 0x4U;

    while (blkCnt > 0U)
    {
        NET_REG_DATA = *SendData++;
        blkCnt--;
    }

	iow(DM9000_REG_TCR, DM9000_TCR_SET);				/* ���д��� */

#ifdef FifoPointCheck
	if(calc_MWR != ((ior(DM9000_REG_MWRH) << 8) + ior(DM9000_REG_MWRL)))
	{
#ifdef Point_Error_Reset
		/* ����ָ��������ȴ���һ������� , ֮��������� */
		while(ior(DM9000_REG_TCR) & DM9000_TCR_SET) dm9k_udelay (5);
		dm9k_reset();
		return;
#endif
		/*����ָ���������ָ���Ƶ���һ�����Ͱ��İ�ͷλ��  */
		iow(DM9000_REG_MWRH, (calc_MWR >> 8) & 0xff);
		iow(DM9000_REG_MWRL, calc_MWR & 0xff);
	}
#endif
}

/*
*********************************************************************************************************
*	������: dm9k_interrupt
*	��  ��: ��
*	��  ��: ��
*	��  ��: �жϴ�������
*********************************************************************************************************
*/
__weak void dm9k_re_packet(void)
{
	/* ��CMSIS-Driver�е��� */
}
void  dm9k_interrupt(void)
{
	uint8_t  save_reg;
	uint8_t  isr_status;

	save_reg = NET_REG_ADDR;						/* �ݴ���ʹ�õ�λ�� */
			
	iow(DM9000_REG_IMR , DM9000_IMR_OFF);			/* �ر� DM9000A �ж� */
	isr_status = ior(DM9000_REG_ISR);				/* ȡ���жϲ���ֵ */
	iow(DM9000_REG_ISR, isr_status);	

	if(isr_status & DM9000_RX_INTR) 				/* ����Ƿ�Ϊ�����ж� */
		dm9k_re_packet();							/* ִ�н��մ������� */

	iow(DM9000_REG_IMR , DM9000_IMR_SET);			/* ���� DM9000A �ж� */
	NET_REG_ADDR = save_reg;						/* �ظ���ʹ�õ�λ�� */
}

/*
*********************************************************************************************************
*	�� �� ��: EXTI9_5_IRQHandler
*	����˵��: �ⲿ�жϷ���������
*	��    ��: ��
*	�� �� ֵ: ��
*********************************************************************************************************
*/
void BUSY_IRQHandler(void)
{
	HAL_GPIO_EXTI_IRQHandler(BUSY_PIN);
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if (GPIO_Pin == BUSY_PIN)
	{
		dm9k_interrupt();
	}
}

/*
*********************************************************************************************************
*	������: etherdev_init
*	��  ��: ��
*	��  ��: ��
*	��  ��: ��ʼ������
*********************************************************************************************************
*/
void etherdev_init(void)
{
	DM9K_CtrlLinesConfig();
	DM9K_FSMCConfig();

	s_FSMC_Init_Ok = 1;

	dm9k_initnic();
}

/*
*********************************************************************************************************
*	������: etherdev_send
*	��  ��: p_char : ���ݻ�����
*			length : ���ݳ���
*	��  ��: ��
*	��  ��: ����һ������
*********************************************************************************************************
*/
void etherdev_send(uint8_t *p_char, uint16_t length)
{
	dm9k_send_packet(p_char, length);
}

uint16_t etherdev_read(uint8_t *p_char)
{
	return dm9k_receive_packet(p_char);
}

/*
*********************************************************************************************************
*	������: etherdev_chkmedia
*	��  ��: p_char : ���ݻ�����
*			length : ���ݳ���
*	��  ��: ��
*	��  ��: �����������״̬
*********************************************************************************************************
*/
void etherdev_chkmedia(void)
{
	while(!(ior(DM9000_REG_NSR) & DM9000_PHY))
	{
		dm9k_udelay(2000);
	}
}

/*
*********************************************************************************************************
*	������: dm9k_ReadID
*	��  ��: ��
*	��  ��: ��
*	��  ��: ��ȡоƬID
*********************************************************************************************************
*/
uint32_t dm9k_ReadID(void)
{
	uint8_t vid1,vid2,pid1,pid2;

	if (s_FSMC_Init_Ok == 0)
	{
		DM9K_CtrlLinesConfig();
		DM9K_FSMCConfig();

		s_FSMC_Init_Ok = 1;
	}
	vid1 = ior(DM9000_REG_VID_L) & 0xFF;
	vid2 = ior(DM9000_REG_VID_H) & 0xFF;
	pid1 = ior(DM9000_REG_PID_L) & 0xFF;
	pid2 = ior(DM9000_REG_PID_H) & 0xFF;

	return (vid2 << 24) | (vid1 << 16) | (pid2 << 8) | pid1;
}

/*
*********************************************************************************************************
*	������: DM9000AE_CtrlLinesConfig
*	��  ��: ��
*	��  ��: ��
*	��  ��: ����DM9000AE���ƿ��ߣ�FSMC�ܽ�����Ϊ���ù���
*********************************************************************************************************
*/
static void DM9K_CtrlLinesConfig(void)
{
	/*
		������STM32-V5��������߷�����
	
		PD0/FSMC_D2
		PD1/FSMC_D3
		PD4/FSMC_NOE		--- �������źţ�OE = Output Enable �� N ��ʾ����Ч
		PD5/FSMC_NWE		--- д�����źţ�WE = Output Enable �� N ��ʾ����Ч

		PD8/FSMC_D13
		PD9/FSMC_D14
		PD10/FSMC_D15
		PD13/FSMC_A18		---- ��ַ CMD
		PD14/FSMC_D0
		PD15/FSMC_D1

		PE4/FSMC_A20		--- ����Ƭѡһ������
		PE5/FSMC_A21		--- ����Ƭѡһ������
		PE7/FSMC_D4
		PE8/FSMC_D5
		PE9/FSMC_D6
		PE10/FSMC_D7
		PE11/FSMC_D8
		PE12/FSMC_D9
		PE13/FSMC_D10
		PE14/FSMC_D11
		PE15/FSMC_D12

		PG10/FSMC_NE3		--- ��Ƭѡ��DM9000AEP �� SRAM��

		PA15/DM9000_INT		--- DM9000AEP �ж�
		
		PD13
	
	*/
    GPIO_InitTypeDef gpio_init_structure;

	/* ʹ�� GPIOʱ�� */
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();
	__HAL_RCC_GPIOH_CLK_ENABLE();
	__HAL_RCC_GPIOI_CLK_ENABLE();

	/* ʹ��FMCʱ�� */
	__HAL_RCC_FSMC_CLK_ENABLE();

	/* ���� GPIOD ��ص�IOΪ����������� */
	gpio_init_structure.Mode = GPIO_MODE_AF_PP;
	gpio_init_structure.Pull = GPIO_PULLUP;
	gpio_init_structure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
	gpio_init_structure.Alternate = GPIO_AF12_FSMC;
	
	/* ����GPIOD */
	gpio_init_structure.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5  |
	                            GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_13 | GPIO_PIN_14 |
	                            GPIO_PIN_15;
	HAL_GPIO_Init(GPIOD, &gpio_init_structure);

	/* ����GPIOE */
	gpio_init_structure.Pin = GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
	                          GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
	HAL_GPIO_Init(GPIOE, &gpio_init_structure);

	/* ����GPIOG */
	gpio_init_structure.Pin = GPIO_PIN_10;
	HAL_GPIO_Init(GPIOG, &gpio_init_structure);

	{
		GPIO_InitTypeDef   GPIO_InitStructure;
	
		BUSY_RCC_GPIO_CLK_ENABLE();
		__HAL_RCC_SYSCFG_CLK_ENABLE();

		GPIO_InitStructure.Mode = GPIO_MODE_IT_FALLING;	/* �ж��½��ش��� */
		GPIO_InitStructure.Pull = GPIO_NOPULL;
		GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_VERY_HIGH;		
		GPIO_InitStructure.Pin = BUSY_PIN;
		HAL_GPIO_Init(BUSY_GPIO, &GPIO_InitStructure);	

		HAL_NVIC_SetPriority(BUSY_IRQn, 0, 0);
		HAL_NVIC_EnableIRQ(BUSY_IRQn);
	}
}

/*
*********************************************************************************************************
*	������: DM9K_FSMCConfig
*	��  ��: ��
*	��  ��: ��
*	��  ��: ����FSMC���ڷ���ʱ��
*********************************************************************************************************
*/
static void DM9K_FSMCConfig(void)
{
	/* 
	   TFT-LCD��OLED��AD7606����һ��FSMC���ã����������������FSMC�ٶ�������Ϊ׼��
	   �Ӷ���֤�������趼��������������
	*/
	SRAM_HandleTypeDef hsram = {0};
	FMC_NORSRAM_TimingTypeDef SRAM_Timing = {0};
		
	hsram.Instance  = FMC_NORSRAM_DEVICE;
	hsram.Extended  = FMC_NORSRAM_EXTENDED_DEVICE;
	
    /* FMCʹ�õ�HCLK����Ƶ168MHz��1��FMCʱ�����ھ���5.95ns */
	SRAM_Timing.AddressSetupTime       = 3;  /* 3*5.95ns=17.85ns����ַ����ʱ�䣬��Χ0 -15��FMCʱ�����ڸ��� */
	SRAM_Timing.AddressHoldTime        = 0;  /* ��ַ����ʱ�䣬����ΪģʽAʱ���ò����˲��� ��Χ1 -15��ʱ�����ڸ��� */
	SRAM_Timing.DataSetupTime          = 6;  /* 6*5.95ns=35.7ns�����ݱ���ʱ�䣬��Χ1 -255��ʱ�����ڸ��� */
	SRAM_Timing.BusTurnAroundDuration  = 8;  /* ������������֮���ʱ��������Χ1 -15��ʱ�����ڸ��� */
	SRAM_Timing.CLKDivision            = 0;  /* �������ò���������� */
	SRAM_Timing.DataLatency            = 0;  /* �������ò���������� */
	SRAM_Timing.AccessMode             = FSMC_ACCESS_MODE_A; /* ����ΪģʽA */

	hsram.Init.NSBank             = FSMC_NORSRAM_BANK3;              /* ʹ�õ�BANK4����ʹ�õ�ƬѡFSMC_NE4 */
	hsram.Init.DataAddressMux     = FSMC_DATA_ADDRESS_MUX_DISABLE;   /* ��ֹ��ַ���ݸ��� */
	hsram.Init.MemoryType         = FSMC_MEMORY_TYPE_SRAM;           /* �洢������SRAM */
	hsram.Init.MemoryDataWidth    = FSMC_NORSRAM_MEM_BUS_WIDTH_16;	 /* 16λ���߿��� */
	hsram.Init.BurstAccessMode    = FSMC_BURST_ACCESS_MODE_DISABLE;  /* �ر�ͻ��ģʽ */
	hsram.Init.WaitSignalPolarity = FSMC_WAIT_SIGNAL_POLARITY_LOW;   /* �������õȴ��źŵļ��ԣ��ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.WaitSignalActive   = FSMC_WAIT_TIMING_BEFORE_WS;      /* �ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.WriteOperation     = FSMC_WRITE_OPERATION_ENABLE;     /* ����ʹ�ܻ��߽�ֹд���� */
	hsram.Init.WaitSignal         = FSMC_WAIT_SIGNAL_DISABLE;        /* �ر�ͻ��ģʽ���˲�����Ч */
	hsram.Init.ExtendedMode       = FSMC_EXTENDED_MODE_DISABLE;      /* ��ֹ��չģʽ */
	hsram.Init.AsynchronousWait   = FSMC_ASYNCHRONOUS_WAIT_DISABLE;  /* �����첽�����ڼ䣬ʹ�ܻ��߽�ֹ�ȴ��źţ�����ѡ��ر� */
	hsram.Init.WriteBurst         = FSMC_WRITE_BURST_DISABLE;        /* ��ֹдͻ�� */
	hsram.Init.ContinuousClock    = FSMC_CONTINUOUS_CLOCK_SYNC_ONLY; /* ��ͬ��ģʽ����ʱ����� */
    hsram.Init.WriteFifo          = FSMC_WRITE_FIFO_ENABLE;          /* ʹ��дFIFO */

	/* ��ʼ��SRAM������ */
	if (HAL_SRAM_Init(&hsram, &SRAM_Timing, &SRAM_Timing) != HAL_OK)
	{
		/* ��ʼ������ */
		Error_Handler(__FILE__, __LINE__);
	}	
}

/***************************** ���������� www.armfly.com (END OF FILE) *********************************/