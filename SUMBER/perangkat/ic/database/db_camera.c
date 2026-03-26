/*
 * PIGURA OS - DATABASE/DB_CAMERA.C
 * ================================
 * Database Camera/Webcam Controllers
 *
 * Versi: 1.0
 */

#include "../ic.h"

status_t ic_muat_camera_database(void)
{
    ic_entri_t *entri;
    status_t hasil;
    
    /* USB Video Class (UVC) Cameras */
    entri = ic_entri_buat("UVC Camera Generic", "Generic", "USB Video Class", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x0000; entri->device_id = 0x0000; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Logitech Webcams */
    entri = ic_entri_buat("Logitech C920 HD Pro", "Logitech", "C920", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x082D; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Logitech C922 Pro Stream", "Logitech", "C922", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x085C; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Logitech C930e Business", "Logitech", "C930e", IC_KATEGORI_CAMERA, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x0843; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Logitech Brio 4K", "Logitech", "Brio", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x085E; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Logitech C270 HD", "Logitech", "C270", IC_KATEGORI_CAMERA, IC_RENTANG_LOW_END);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x0825; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Logitech C310", "Logitech", "C310", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x046D; entri->device_id = 0x081B; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Microsoft Webcams */
    entri = ic_entri_buat("Microsoft LifeCam HD-3000", "Microsoft", "HD-3000", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x045E; entri->device_id = 0x0779; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Microsoft LifeCam Studio", "Microsoft", "Studio", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x045E; entri->device_id = 0x0812; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Razer Webcams */
    entri = ic_entri_buat("Razer Kiyo Pro", "Razer", "Kiyo Pro", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x1532; entri->device_id = 0x0E02; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Razer Kiyo", "Razer", "Kiyo", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x1532; entri->device_id = 0x0E01; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Elgato Webcams */
    entri = ic_entri_buat("Elgato Facecam", "Elgato", "Facecam", IC_KATEGORI_CAMERA, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x0FD9; entri->device_id = 0x006B; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Elgato Facecam Pro", "Elgato", "Facecam Pro", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x0FD9; entri->device_id = 0x0088; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Lenovo Webcams */
    entri = ic_entri_buat("Lenovo EasyCamera", "Lenovo", "EasyCamera", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x13D3; entri->device_id = 0x5406; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Lenovo ThinkPad Camera", "Lenovo", "ThinkPad", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x04F2; entri->device_id = 0xB6EB; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* Dell Webcams */
    entri = ic_entri_buat("Dell UltraSharp Webcam", "Dell", "UltraSharp WB7022", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x413C; entri->device_id = 0xB71E; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Dell Integrated Webcam", "Dell", "Integrated", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x0C45; entri->device_id = 0x671B; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* HP Webcams */
    entri = ic_entri_buat("HP TrueVision HD", "HP", "TrueVision HD", IC_KATEGORI_CAMERA, IC_RENTANG_MID_LOW);
    if (entri) { entri->vendor_id = 0x04F2; entri->device_id = 0xB40A; entri->bus = IC_BUS_USB; hasil = ic_database_tambah(entri); }
    
    /* IPU Camera Sensors (Mobile/Embedded) */
    entri = ic_entri_buat("Sony IMX586", "Sony", "IMX586", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x10EC; entri->device_id = 0x0586; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Sony IMX686", "Sony", "IMX686", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x10EC; entri->device_id = 0x0686; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Sony IMX766", "Sony", "IMX766", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x10EC; entri->device_id = 0x0766; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Sony IMX989", "Sony", "IMX989", IC_KATEGORI_CAMERA, IC_RENTANG_ENTHUSIAST);
    if (entri) { entri->vendor_id = 0x10EC; entri->device_id = 0x0989; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Samsung ISOCELL HM3", "Samsung", "HM3", IC_KATEGORI_CAMERA, IC_RENTANG_HIGH_END);
    if (entri) { entri->vendor_id = 0x144D; entri->device_id = 0x2003; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("Samsung ISOCELL GN1", "Samsung", "GN1", IC_KATEGORI_CAMERA, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x144D; entri->device_id = 0x1001; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("OmniVision OV48C", "OmniVision", "OV48C", IC_KATEGORI_CAMERA, IC_RENTANG_MID_HIGH);
    if (entri) { entri->vendor_id = 0x3674; entri->device_id = 0x48C0; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    entri = ic_entri_buat("OmniVision OV64B", "OmniVision", "OV64B", IC_KATEGORI_CAMERA, IC_RENTANG_MID);
    if (entri) { entri->vendor_id = 0x3674; entri->device_id = 0x64B0; entri->bus = IC_BUS_I2C; hasil = ic_database_tambah(entri); }
    
    return STATUS_BERHASIL;
}
