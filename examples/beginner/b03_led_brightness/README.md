# ควบคุมความสว่าง LED (LED Brightness Control)

ตัวอย่างนี้สาธิตการเชื่อมต่อ Slider widget กับ Virtual LED ของ LVGL การเลื่อน Slider จะเปลี่ยนความสว่างของ LED แบบ real-time พร้อม Label แสดงเปอร์เซ็นต์ความสว่างปัจจุบัน

ช่วงค่าของ Slider ตั้งจาก 0 ถึง 255 ตรงกับสเกลความสว่างของ LVGL LED เมื่อค่า Slider เปลี่ยน callback `LV_EVENT_VALUE_CHANGED` จะอ่านค่าใหม่และส่งไปยัง `lv_led_set_brightness` พร้อมอัปเดต Label เปอร์เซ็นต์ไปพร้อมกัน

Virtual LED widget ใน LVGL จำลอง LED จริงที่ปรับความสว่างและสีได้ แสดงผลเป็นวงกลมแบบ gradient ที่หรี่หรือสว่างตามค่าที่กำหนด เหมาะสำหรับการสร้างต้นแบบ interface ควบคุมฮาร์ดแวร์ก่อนเชื่อมต่ออุปกรณ์จริง

ตัวอย่างนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit เนื่องจากใช้เพียง LVGL widget โดยไม่พึ่งพาฮาร์ดแวร์
