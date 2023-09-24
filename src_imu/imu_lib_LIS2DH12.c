//*****************************************************************************
//*		imu_LIS2DH12_lib.c
//*
//*		Compile with gcc imu_LIS2DH12_lib.c -li2c -lm
//*****************************************************************************
//*	Edit History
//*****************************************************************************
//*	<MLS>	=	Mark L Sproul
//*****************************************************************************
//*	Jul  7,	2023	<MLS> Created imu_LIS2DH12_lib.c
//*	Jul 31,	2023	<CRN> Added read and write functions, test functionality.
//*	Aug  3,	2023	<MLS> Compile with -li2c -lm
//*	Sep 11,	2023	<MLS> Started integrating LIS2DH12 imu support into AlpacaPi
//*****************************************************************************


#include	<stdio.h>
#include	<unistd.h>
#include 	<fcntl.h>
#include 	<sys/ioctl.h>
#include	<math.h>
#include 	<linux/i2c-dev.h>
#include 	<i2c/smbus.h>


#define _ENABLE_CONSOLE_DEBUG_
#include	"ConsoleDebug.h"


#include	"imu_lib_LIS2DH12.h"
#include	"lis2dh12_reg.h"
#include	"lis2dh12_reg.c"

#define	BOOT_TIME		5	//ms

#define	DEGREES(radians)	((radians) * (180.0 / M_PI))
#define I2CBUS				"/dev/i2c-1"

static	int	gI2CfileDecriptor;				// I2C file descriptor


//*****************************************************************************
//*	for reference from smbus.h
//*	extern __s32 i2c_smbus_write_byte_data(int file, __u8 command, __u8 value);
//*****************************************************************************

//*****************************************************************************
//*	Open IC2 device
//*	returns >= 0 file descriptor, < 0 on failure
//*****************************************************************************
static int	I2C_OpenDevice(char *i2cbus, uint8_t i2cAddress)
{
int		i2cFileDecriptor;						// I2C file descriptor

	CONSOLE_DEBUG(__FUNCTION__);
	i2cFileDecriptor	=	open(i2cbus, O_RDWR);
	if (i2cFileDecriptor >= 0)
	{
		if (ioctl(i2cFileDecriptor, I2C_SLAVE, i2cAddress) != 0)
		{
			printf("Error can't find sensor at address [0x%02X].\n", i2cAddress);
			return(-1);
		}
		//*	I2C communication test is the only way to confirm success
		//---------------------------------------------------------
//		char reg	=	BNO055_CHIP_ID_ADDR;
//		if write(gI2cfd, &reg, 1) != 1)
//		{
//			printf("Error: I2C write failure register [0x%02X], sensor i2cAddress [0x%02X]?\n", reg, i2cAddress);
//			EXIT(-1);
//		}
	}
	else
	{
		printf("Error failed to open I2C bus [%s].\n", i2cbus);
		return(-1);
	}

	//---------------------------------------------------------
	return(i2cFileDecriptor);
}

//*****************************************************************************
int	IMU_LIS2DH12_Init(void)
{
int		returnCode;		//*	0 means OK
int		lis2dh12addr	=	0x19;

	gI2CfileDecriptor	=	I2C_OpenDevice(I2CBUS, lis2dh12addr);
	if (gI2CfileDecriptor >= 0)
	{
		CONSOLE_DEBUG_W_NUM("i2cFileDecriptor\t=", gI2CfileDecriptor);
		returnCode	=	0;
	}
	else
	{
		CONSOLE_DEBUG_W_HEX("Failed to open I2C device\t=", lis2dh12addr);
		returnCode	=	-1;
	}
	return(returnCode);
}


//*****************************************************************************
static void platform_init(void)
{

}

//*****************************************************************************
static int32_t platform_write(	void			*handle,
								uint8_t			reg,
								const uint8_t	*bufp,
								uint16_t		len)
{
int		iii;
int		myFileDescriptor;

	myFileDescriptor	=	(int)handle;

	for (iii=0; iii < len; iii++)
	{
		i2c_smbus_write_byte_data(myFileDescriptor, (reg + iii), bufp[iii]);
	}
	return 0;
}

//*****************************************************************************
static	int32_t	platform_read(	void		*handle,
								uint8_t		reg,
								uint8_t		*bufp,
								uint16_t	len)
{
int		iii;
int		myFileDescriptor;

	myFileDescriptor	=	(int)handle;

	for (iii=0; iii < len; iii++)
	{
		bufp[iii]	=	i2c_smbus_read_byte_data(myFileDescriptor, (reg + iii));
	}
	return 0;
}

//*****************************************************************************
static void platform_delay(uint32_t ms)
{
	usleep(ms * 1000);
}

//*****************************************************************************
static void raw_to_angle(int16_t vals[3], double out[3])
{
double	tmp;
int		iii;

	tmp	=	sqrtf((vals[0] * vals[0]) + (vals[1] * vals[1]) + (vals[2] * vals[2]));
	for (iii=0; iii < 3; iii++)
	{
		out[iii]	=	acosf(vals[iii] / tmp);
	}
}

#ifdef _INCLUDE_IMU_MAIN_


//*****************************************************************************
int main(int argc, char *argv[])
{
//int				imu_mode;
//int				returnCode;
//double			heading;
//double			roll;
//double			pitch;
//double			myRoll_rad;
//double			myPitch_rad;

int16_t			raw_acc[3];
float			acc_mg[3];
double			angle[3];
int				lis2dh12ReturnCode;
stmdev_ctx_t	dev_ctx;
lis2dh12_reg_t	reg;
int				i2cFileDecriptor;				// I2C file descriptor
int				lis2dh12addr	=	0x19;
int				count			=	0;

	printf("IMU LIS2DH12 test\r\n");

	i2cFileDecriptor	=	I2C_OpenDevice(I2CBUS, lis2dh12addr);
	if (i2cFileDecriptor >= 0)
	{
		printf("i2cFileDecriptor\t=%d\r\n", i2cFileDecriptor);
	}
	else
	{
		printf("Failed to open I2C device=%X\r\n", lis2dh12addr);
	}

	//* Initialize mems driver interface
	dev_ctx.write_reg	=	platform_write;
	dev_ctx.read_reg	=	platform_read;
//	dev_ctx.handle		=	&SENSOR_BUS;
	dev_ctx.handle		=	(void *)i2cFileDecriptor;

	//* Wait boot time and initialize platform specific hardware
	platform_init();

	//* Wait sensor boot time
	platform_delay(BOOT_TIME);

	//* Check device ID
	lis2dh12ReturnCode	=	lis2dh12_device_id_get(&dev_ctx, &reg.byte);
	printf("lis2dh12ReturnCode\t=%d\r\n", lis2dh12ReturnCode);

	if (reg.byte != LIS2DH12_ID)
	{
		while ((count < 10) && (reg.byte != LIS2DH12_ID))
		{
			printf("Invalid device. Got ID: #%d\tAttempt #%d\n", reg.byte, count+1);
			platform_delay(1500);
			count++;
			lis2dh12_device_id_get(&dev_ctx, &reg.byte);
			//* manage here device not found
		}
		if ((count == 10) && (reg.byte != LIS2DH12_ID))
		{
			printf("Device not found; exiting.\n");
			return -1;
		}
	}

	lis2dh12_block_data_update_set(	&dev_ctx, PROPERTY_ENABLE);
	lis2dh12_data_rate_set(			&dev_ctx, LIS2DH12_ODR_400Hz);
	lis2dh12_full_scale_set(		&dev_ctx, LIS2DH12_2g);
	lis2dh12_operating_mode_set(	&dev_ctx, LIS2DH12_HR_12bit);
	while (1)
	{
		lis2dh12_xl_data_ready_get(&dev_ctx, &reg.byte);
		if (reg.byte)
		{
			raw_acc[0]	=	0;
			raw_acc[1]	=	0;
			raw_acc[2]	=	0;
			lis2dh12_acceleration_raw_get(&dev_ctx, raw_acc);
//			printf("Got raw acceleration: \t\t%06d, \t%06d, \t%06d\n", raw_acc[0], raw_acc[1], raw_acc[2]);
			acc_mg[0]	=	lis2dh12_from_fs2_hr_to_mg(raw_acc[0]);
			acc_mg[1]	=	lis2dh12_from_fs2_hr_to_mg(raw_acc[1]);
			acc_mg[2]	=	lis2dh12_from_fs2_hr_to_mg(raw_acc[2]);
//			printf("Got acceleration: \t%06.6f, \t%06.6f, \t%06.6f\n", acc_mg[0], acc_mg[1], acc_mg[2]);
			raw_to_angle(raw_acc, angle);
//			printf("Got actual angle (rad): \t%06.6f, \t%06.6f, \t%06.6f\n", angle[0], angle[1], angle[2]);
			printf("Got actual angle (deg): \t%06.6f, \t%06.6f, \t%06.6f\n",	DEGREES(angle[0]),
																				DEGREES(angle[1]),
																				DEGREES(angle[2]));
			platform_delay(1500);
		}
	}
}
#endif // _INCLUDE_IMU_MAIN_


