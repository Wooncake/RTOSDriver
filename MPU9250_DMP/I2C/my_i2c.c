#include "my_i2c.h"


// 位带宏 F1/F4通用
#define BITBAND(addr, bitnum)    ((addr & 0xF0000000) + 0x2000000 + ((addr & 0xFFFFF) << 5) + (bitnum << 2))
#define MEM_ADDR(addr)           *((volatile uint32_t *)(addr))
#define BIT_ADDR(addr, bitnum)   MEM_ADDR(BITBAND(addr, bitnum))

/**
 * @brief iic gpio operate definition STM32F103版本
 */
#define GPIOB_ODR_Addr    (GPIOB_BASE + 0x0C)  //F1 ODR偏移0x0C，F4是0x14
#define GPIOB_IDR_Addr    (GPIOB_BASE + 0x08)  //F1 IDR偏移0x08，F4是0x10
#define PBout(n)          BIT_ADDR(GPIOB_ODR_Addr, n)
#define PBin(n)           BIT_ADDR(GPIOB_IDR_Addr, n)

// PB8 是 SCL，PB9 是 SDA，均位于 CRH。
// 板上已有外部上拉，因此使用开漏输出，符合 I2C 电气规范。
// 读 ACK 和数据时切换为上拉输入，让 MPU9250 驱动 SDA。
// SDA_IN()：上拉输入 CNF=10 MODE=00 -> 0x8。
// SDA_OUT()：开漏输出 CNF=01 MODE=11 -> 0x7。
#define SDA_IN()   do { GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 4)) | (0x8U << 4); } while (0)
#define SDA_OUT()  do { GPIOB->CRH = (GPIOB->CRH & ~(0xFU << 4)) | (0x7U << 4); } while (0)

#define IIC_SCL           PBout(8)
#define IIC_SDA           PBout(9)
#define READ_SCL          PBin(8)
#define READ_SDA          PBin(9)


/**
 * @brief  iic bus init
 * @return status code
 *         - 0 success
 * @note   SCL is PB8 and SDA is PB9
 */
uint8_t iic_init(void)
{
    uint8_t round;
    uint8_t pulse;
    GPIO_InitTypeDef GPIO_Initure;
    
    /* enable iic gpio clock */
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* A previous failed attempt may have left PB8/PB9 in a different mode. */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    delay_us(10);
    
    /* Release SDA first so a reset in the middle of a transfer can be recovered. */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_8 | GPIO_PIN_9, GPIO_PIN_SET);

    GPIO_Initure.Pin = GPIO_PIN_8;
    GPIO_Initure.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_Initure.Pull = GPIO_PULLUP;
    GPIO_Initure.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOB, &GPIO_Initure);

    IIC_SCL = 1;

    GPIO_Initure.Pin = GPIO_PIN_9;
    GPIO_Initure.Mode = GPIO_MODE_INPUT;
    GPIO_Initure.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_Initure);
    IIC_SDA = 1;
    delay_us(20);

    /* Recover a transfer left unfinished by an MCU reset. A slave may keep
       SDA low until it sees clocks followed by a complete STOP condition. */
    for (round = 0U; round < 3U; round++)
    {
        SDA_IN();
        IIC_SDA = 1;
        IIC_SCL = 1;
        delay_us(20);

        if ((READ_SCL != 0U) && (READ_SDA != 0U))
        {
            break;
        }

        for (pulse = 0U; pulse < 9U; pulse++)
        {
            IIC_SCL = 0;
            delay_us(20);
            IIC_SCL = 1;
            delay_us(20);
            if (READ_SCL == 0U)
            {
                break;
            }
        }

        /* Force STOP even when SDA was still low during recovery. */
        SDA_OUT();
        IIC_SDA = 0;
        delay_us(20);
        IIC_SCL = 1;
        delay_us(20);
        IIC_SDA = 1;
        delay_us(50);

        SDA_IN();
        IIC_SDA = 1;
        delay_us(20);
        if ((READ_SCL != 0U) && (READ_SDA != 0U))
        {
            break;
        }
        delay_us(100);
    }

    if ((READ_SCL == 0U) || (READ_SDA == 0U))
    {
        /* Leave the pins in a neutral state before the caller retries. */
        HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
        return 1;
    }

    /* Keep the same push-pull idle mode as the existing bit-bang driver. */
    SDA_OUT();
    IIC_SCL = 1;
    IIC_SDA = 1;
    delay_us(20);
    
    return 0;
}

/**
 * @brief  iic bus deinit
 * @return status code
 *         - 0 success
 * @note   none
 */
uint8_t iic_deinit(void)
{
    /* iic gpio deinit */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_8 | GPIO_PIN_9);
    
    return 0;
}

/**
 * @brief      get the physical bus levels
 * @param[out] *scl pointer to the SCL level
 * @param[out] *sda pointer to the SDA level
 * @note       none
 */
void iic_get_bus_state(uint8_t *scl, uint8_t *sda)
{
    if (scl != NULL)
    {
        *scl = (READ_SCL != 0U) ? 1U : 0U;
    }
    if (sda != NULL)
    {
        *sda = (READ_SDA != 0U) ? 1U : 0U;
    }
}

/**
 * @brief iic bus send start
 * @note  none
 */
static void a_iic_start(void)
{
    SDA_OUT();
    IIC_SCL = 1;
    delay_us(15);
    IIC_SDA = 0;
    delay_us(15);
    IIC_SCL = 0;
    delay_us(15);
    IIC_SDA = 1;
    delay_us(15);
}

/**
 * @brief iic bus send stop
 * @note  none
 */
static void a_iic_stop(void)
{
    SDA_OUT();
    IIC_SDA = 0;
    delay_us(15);
    IIC_SCL = 1;
    delay_us(15);
    IIC_SDA = 1;
    delay_us(15);
}

/**
 * @brief  iic wait ack
 * @return status code
 *         - 0 get ack
 *         - 1 no ack
 * @note   none
 */
static uint8_t a_iic_wait_ack(void)
{
    uint16_t uc_err_time = 0;
    
    SDA_IN();
    IIC_SDA = 1; 
    delay_us(20);
    IIC_SCL = 1; 
    delay_us(20);
    while (READ_SDA != 0)
    {
        uc_err_time++;
        if (uc_err_time > 500)
        {
            a_iic_stop();
            
            return 1;
        }
    }
    IIC_SCL = 0;
    delay_us(20);
    
    return 0;
}

/**
 * @brief iic bus send ack
 * @note  none
 */
static void a_iic_ack(void)
{
    IIC_SCL = 0;
    delay_us(20);
    SDA_OUT();
    IIC_SDA = 0;
    delay_us(20);
    IIC_SCL = 1;
    delay_us(20);
    IIC_SCL = 0;
    delay_us(20);
}

/**
 * @brief iic bus send nack
 * @note  none
 */
static void a_iic_nack(void)
{
    IIC_SCL = 0;
    delay_us(20);
    SDA_OUT();
    IIC_SDA = 1;
    delay_us(20);
    IIC_SCL = 1;
    delay_us(20);
    IIC_SCL = 0; 
    delay_us(20);
}

/**
 * @brief     iic send one byte
 * @param[in] txd sent byte
 * @note      none
 */
static void a_iic_send_byte(uint8_t txd)
{
    uint8_t t;
    
    SDA_OUT();
    IIC_SCL = 0;
    for (t = 0; t < 8; t++)
    {
        IIC_SDA = (txd & 0x80U) >> 7;
        txd <<= 1;
        delay_us(10);
        IIC_SCL = 1;
        delay_us(10);
        IIC_SCL = 0;
        delay_us(10);
    }
}

/**
 * @brief     iic read one byte
 * @param[in] ack sent ack
 * @return    read byte
 * @note      none
 */
static uint8_t a_iic_read_byte(uint8_t ack)
{
    uint8_t i;
    uint8_t receive = 0;
    
    SDA_IN();
    for (i = 0; i < 8; i++)
    {
        IIC_SCL = 0;
        delay_us(10);
        IIC_SCL = 1;
        delay_us(10);
        receive <<= 1;
        if (READ_SDA != 0)
        {
            receive++;
        }
        delay_us(10);
    }
    if (ack != 0)
    {
        a_iic_ack();
    }
    else
    {
        a_iic_nack();
    }
    
    return receive;
}

/**
 * @brief     iic bus write command
 * @param[in] addr iic device write address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      addr = device_address_7bits << 1
 */
uint8_t iic_write_cmd(uint8_t addr, uint8_t *buf, uint16_t len)
{
    uint16_t i; 
    
    /* send a start */
    a_iic_start();
    
    /* send the write addr */
    a_iic_send_byte(addr);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* write the data */
    for (i = 0; i < len; i++)
    {
        /* send one byte */
        a_iic_send_byte(buf[i]);
        if (a_iic_wait_ack() != 0)
        {
            a_iic_stop();
            
            return 1;
        }
    }
    
    /* send a stop */
    a_iic_stop();
    
    return 0;
} 

/**
 * @brief     iic bus write
 * @param[in] addr iic device write address
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      addr = device_address_7bits << 1
 */
uint8_t iic_write(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    uint16_t i; 
    
    /* send a start */
    a_iic_start();
    
    /* send the write addr */
    a_iic_send_byte(addr);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send the reg */
    a_iic_send_byte(reg);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* write the data */
    for (i = 0; i < len; i++)
    {
        /* send one byte */
        a_iic_send_byte(buf[i]);
        if (a_iic_wait_ack() != 0)
        {
            a_iic_stop(); 
            
            return 1;
        }
    }
    
    /* send a stop */
    a_iic_stop();
    
    return 0;
} 

/**
 * @brief     iic bus write with 16 bits register address 
 * @param[in] addr iic device write address
 * @param[in] reg iic register address
 * @param[in] *buf pointer to a data buffer
 * @param[in] len length of the data buffer
 * @return    status code
 *            - 0 success
 *            - 1 write failed
 * @note      addr = device_address_7bits << 1
 */
uint8_t iic_write_address16(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len)
{
    uint16_t i; 
    
    /* send a start */
    a_iic_start();
    
    /* send the write addr */
    a_iic_send_byte(addr);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send the reg high part */
    a_iic_send_byte((reg >> 8) & 0xFF);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send the reg low part */
    a_iic_send_byte(reg & 0xFF);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* write the data */
    for (i = 0; i < len; i++)
    {
        /* send one byte */
        a_iic_send_byte(buf[i]);
        if (a_iic_wait_ack() != 0)
        {
            a_iic_stop();
            
            return 1;
        }
    }
    
    /* send a stop */
    a_iic_stop();
    
    return 0;
} 

/**
 * @brief      iic bus read command
 * @param[in]  addr iic device write address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       addr = device_address_7bits << 1
 */
uint8_t iic_read_cmd(uint8_t addr, uint8_t *buf, uint16_t len)
{
    /* send a start */
    a_iic_start();
    
    /* send the read addr */
    a_iic_send_byte(addr + 1);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* read the data */
    while (len != 0)
    {
        /* if the last */
        if (len == 1)
        {
            /* send nack */
            *buf = a_iic_read_byte(0);
        }
        else
        {
            /* send ack */
            *buf = a_iic_read_byte(1); 
        }
        len--;
        buf++;
    }
    
    /* send a stop */
    a_iic_stop(); 
    
    return 0;
}

/**
 * @brief      iic bus read
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       addr = device_address_7bits << 1
 */
uint8_t iic_read(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    /* send a start */
    a_iic_start();
    
    /* send the write addr */
    a_iic_send_byte(addr);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;               /* write address was not acknowledged */
    }
    
    /* send the reg */
    a_iic_send_byte(reg);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 2;               /* register address was not acknowledged */
    }

    /* send a start */
    a_iic_start();
    
    /* send the read addr */
    a_iic_send_byte(addr + 1);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 3;               /* read address was not acknowledged */
    }
    
    /* read the data */
    while (len != 0)
    {
        /* if the last */
        if (len == 1)
        {
            /* send nack */
            *buf = a_iic_read_byte(0);
        }
        else
        {
            /* send ack */
            *buf = a_iic_read_byte(1);
        }
        len--;
        buf++;
    }
    
    /* send a stop */
    a_iic_stop();
    
    return 0;
}

/**
 * @brief      iic bus read with 16 bits register address 
 * @param[in]  addr iic device write address
 * @param[in]  reg iic register address
 * @param[out] *buf pointer to a data buffer
 * @param[in]  len length of the data buffer
 * @return     status code
 *             - 0 success
 *             - 1 read failed
 * @note       addr = device_address_7bits << 1
 */
uint8_t iic_read_address16(uint8_t addr, uint16_t reg, uint8_t *buf, uint16_t len)
{
    /* send a start */
    a_iic_start();
    
    /* send the write addr */
    a_iic_send_byte(addr);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send the reg high part */
    a_iic_send_byte((reg >> 8) & 0xFF);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send the reg low part */
    a_iic_send_byte(reg & 0xFF);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* send a start */
    a_iic_start();
    
    /* send the read addr */
    a_iic_send_byte(addr + 1);
    if (a_iic_wait_ack() != 0)
    {
        a_iic_stop();
        
        return 1;
    }
    
    /* read the data */
    while (len != 0)
    {
        /* if the last */
        if (len == 1)
        {
            /* send nack */
            *buf = a_iic_read_byte(0);
        }
        else
        {
            /* send ack */
            *buf = a_iic_read_byte(1);
        }
        len--;
        buf++;
    }
    
    /* send a stop */
    a_iic_stop();
    
    return 0;
}

