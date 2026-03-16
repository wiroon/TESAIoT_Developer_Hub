# ตัวนับเวลาทำงาน (Timer Uptime Counter)

ตัวอย่างนี้ใช้ LVGL timer เพื่อสร้างตัวนับเวลาทำงานที่อัปเดตทุกวินาที การแสดงผลแสดงชั่วโมง นาที และวินาทีในรูปแบบนาฬิกาดิจิทัล พร้อมจำนวนวินาทีรวมที่ผ่านไป

timer callback ถูกลงทะเบียนด้วย `lv_timer_create` ที่ interval 1000ms ทุกครั้งที่เรียกจะเพิ่มตัวนับวินาทีและจัดรูปแบบข้อความเวลา timer user data pointer พกโครงสร้างบริบทที่มี label reference และค่าตัวนับ

LVGL timer เป็นวิธีที่ถูกต้องสำหรับอัปเดตเป็นระยะภายใน framework ของ LVGL ทำงานใน context ของ LVGL task หลัก ดังนั้นการอัปเดต widget ทั้งหมดใน callback จึงปลอดภัยต่อ thread ห้ามใช้ FreeRTOS timer หรือ `vTaskDelay` สำหรับอัปเดต LVGL widget

รูปแบบนี้เป็นรากฐานสำหรับ dashboard แบบ real-time ที่ต้อง poll เซ็นเซอร์เป็นระยะ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
