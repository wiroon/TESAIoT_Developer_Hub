# การ์ดเซ็นเซอร์แบบ Grid (Sensor Grid Cards)

ตัวอย่างนี้สร้าง grid ของการ์ดข้อมูลเซ็นเซอร์แบบ responsive แต่ละการ์ดแสดงตัวอักษรไอคอน ชื่อเซ็นเซอร์ และค่าสด grid ปรับตัวตามบอร์ดโดยใช้ BSP feature flag แสดงเฉพาะเซ็นเซอร์ที่มีจริงบนฮาร์ดแวร์ที่เชื่อมต่อ

การ์ดจัดเรียงด้วยระบบ grid layout ของ LVGL โดยใช้ `lv_obj_set_grid_dsc_array` grid ใช้ 3 คอลัมน์ความกว้างเท่ากันและแถวตามจำนวนเซ็นเซอร์ที่มี แต่ละการ์ดวางด้วย `lv_obj_set_grid_cell` สำหรับตำแหน่ง grid ที่แม่นยำ

แต่ละการ์ดเซ็นเซอร์มีตัวอักษรไอคอนขนาดใหญ่ (ใช้ LVGL symbol หรือ Unicode) label ชื่อเซ็นเซอร์ และ label ค่าที่อัปเดตจาก timer การ์ดใช้ BENTO dark theme พร้อมสีเน้นเฉพาะเซ็นเซอร์ตรงกับ dashboard ดั้งเดิม

BSP guard (`BSP_HAS_DPS368`, `BSP_HAS_SHT40`, `BSP_HAS_CAPSENSE` เป็นต้น) รวมการ์ดเซ็นเซอร์แบบมีเงื่อนไข BMI270 accelerometer มีในทุกบอร์ดจึงแสดงเสมอ เซ็นเซอร์อื่นถูกคอมไพล์แบบมีเงื่อนไข สาธิตรูปแบบที่ถูกต้องสำหรับ firmware หลายบอร์ด

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
