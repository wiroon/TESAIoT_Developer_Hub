# ปุ่มนับจำนวน (Button Counter)

ตัวอย่างนี้สร้างปุ่มที่สามารถโต้ตอบได้ โดยนับจำนวนครั้งที่ถูกกด Label ด้านล่างปุ่มแสดงจำนวนการกดปัจจุบัน อัปเดตแบบ real-time ทุกครั้งที่กด

ตัวนับการกดใช้ระบบ Event ของ LVGL โดยลงทะเบียน event callback บนปุ่มสำหรับ `LV_EVENT_CLICKED` ทุกครั้งที่ callback ถูกเรียก ตัวแปร counter แบบ static จะเพิ่มค่า และข้อความ Label จะอัปเดตผ่าน `lv_label_set_text_fmt` สำหรับการแสดงผลแบบ formatted

กลไก user data pointer ส่ง label object ไปยัง callback function ทำให้ callback สามารถอัปเดต widget ที่ถูกต้องได้โดยไม่ต้องใช้ตัวแปร global นี่คือรูปแบบที่แนะนำสำหรับการจัดการ Event ใน LVGL

ตัวอย่างนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit เนื่องจากใช้เพียง touch input และ LVGL widget พื้นฐาน
