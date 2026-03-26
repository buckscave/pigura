/*
 * PIGURA OS - DATABASE/DB_SENSOR.C
 * =================================
 * Database Sensors (IMU, Proximity, Ambient Light, etc)
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_sensor_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* IMU Sensors (Accelerometer + Gyro) */
    entri = ic_entri_buat("InvenSense MPU6050", "InvenSense", "MPU6050", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1111; entri->device_id = 0x6050; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("InvenSense MPU9250", "InvenSense", "MPU9250", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1111; entri->device_id = 0x9250; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("InvenSense ICM-20948", "InvenSense", "ICM-20948", IC_KATEGORI_SENSOR, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x1111; entri->device_id = 0x0948; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Bosch BMI160", "Bosch", "BMI160", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x02B0; entri->device_id = 0x0160; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Bosch BMI260", "Bosch", "BMI260", IC_KATEGORI_SENSOR, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x02B0; entri->device_id = 0x0260; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST LSM6DS3", "ST", "LSM6DS3", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x6A3A; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST LSM6DSO", "ST", "LSM6DSO", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x6C32; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    /* Accelerometer Only */
    entri = ic_entri_buat("ADI ADXL345", "ADI", "ADXL345", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0xE5A; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ADI ADXL362", "ADI", "ADXL362", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x00AD; entri->device_id = 0x362; entri->bus = IC_BUS_SPI; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("NXP MMA8452", "NXP", "MMA8452Q", IC_KATEGORI_SENSOR, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x1131; entri->device_id = 0x1C; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Magnetometer */
    entri = ic_entri_buat("Honeywell HMC5883L", "Honeywell", "HMC5883L", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1337; entri->device_id = 0x5883; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Asahi AK8975", "Asahi Kasei", "AK8975", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x0C48; entri->device_id = 0x0C; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Ambient Light Sensors */
    entri = ic_entri_buat("ROHM BH1750", "ROHM", "BH1750FVI", IC_KATEGORI_SENSOR, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x10EC; entri->device_id = 0x1750; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom APDS9301", "Broadcom", "APDS9301", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x04B4; entri->device_id = 0x0301; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("AMS TSL2561", "AMS", "TSL2561", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x2561; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("AMS TSL2591", "AMS", "TSL2591", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x2591; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("ST VL6180X", "ST", "VL6180X", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x6180; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Proximity Sensors */
    entri = ic_entri_buat("Vishay VCNL4010", "Vishay", "VCNL4010", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1347; entri->device_id = 0x4010; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Broadcom APDS9960", "Broadcom", "APDS9960", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x04B4; entri->device_id = 0x0960; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Semtech SX9500", "Semtech", "SX9500", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1C58; entri->device_id = 0x9500; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Temperature/Humidity/Pressure Sensors */
    entri = ic_entri_buat("Bosch BME280", "Bosch", "BME280", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x02B0; entri->device_id = 0x0280; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Bosch BMP280", "Bosch", "BMP280", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x02B0; entri->device_id = 0x0580; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Bosch BME680", "Bosch", "BME680", IC_KATEGORI_SENSOR, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x02B0; entri->device_id = 0x0680; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TI HDC1080", "TI", "HDC1080", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x1000; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("TE MS5637", "TE", "MS5637", IC_KATEGORI_SENSOR, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x1313; entri->device_id = 0x5637; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    /* Hall Effect Sensors */
    entri = ic_entri_buat("AMS AS5600", "AMS", "AS5600", IC_KATEGORI_SENSOR, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1044; entri->device_id = 0x3600; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
