# การอ่านเซ็นเซอร์อัตโนมัติ (Sensor Auto-Poll)

ตัวอย่างนี้สาธิตการใช้ FreeRTOS software timer (`xTimerCreate`) แทน LVGL timer สำหรับการอ่านเซ็นเซอร์แบบคาบ FreeRTOS timer ทำงานใน timer daemon task ให้จังหวะเวลาที่แม่นยำกว่าและไม่ขึ้นกับรอบ refresh ของ LVGL

FreeRTOS timer ทำงานทุก 50ms อ่าน BMI270 accelerometer และเก็บค่าล่าสุดใน volatile structure ที่ใช้ร่วม LVGL timer แยกต่างหาก (200ms) รับข้อมูลเซ็นเซอร์ล่าสุดและอัปเดตกราฟกับ label วิธีนี้แยกการรับข้อมูลเซ็นเซอร์ออกจากการ render UI

label สถานะด้านบนแสดงจำนวนการอ่านเซ็นเซอร์ทั้งหมดและ timestamp การอ่านล่าสุด (tick count) ยืนยันว่า FreeRTOS timer ทำงานอิสระ กราฟอัปเดตน้อยกว่าการอ่านเซ็นเซอร์ ซึ่งเป็นความตั้งใจเพื่อลดภาระ GPU

รูปแบบ timer คู่นี้ (FreeRTOS สำหรับการรับข้อมูล, LVGL สำหรับการแสดงผล) เป็นแนวทางมาตรฐานที่ใช้ใน `sensor_auto_task` ของ firmware จริง ทำให้ข้อมูลเซ็นเซอร์สดใหม่เสมอโดยไม่ผูกกับอัตราเฟรมของจอแสดงผล

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
