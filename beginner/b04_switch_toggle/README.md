# สวิตช์เปิดปิด (Switch Toggle)

ตัวอย่างนี้จับคู่ Switch widget กับ Virtual LED เพื่อสาธิตการควบคุมสถานะแบบ boolean การสลับสวิตช์จะเปิดหรือปิด LED พร้อม Label แสดงสถานะปัจจุบัน

Switch widget ส่ง `LV_EVENT_VALUE_CHANGED` ทุกครั้งที่ถูกสลับ callback ตรวจสอบ `lv_obj_has_state(switch, LV_STATE_CHECKED)` เพื่อระบุตำแหน่งปัจจุบัน จากนั้นเรียก `lv_led_on` หรือ `lv_led_off` ตามสถานะ

รูปแบบเปิด/ปิดนี้ใช้กันทั่วไปใน IoT dashboard สำหรับควบคุมรีเลย์ มอเตอร์ และ actuator แบบ binary อื่นๆ Virtual LED ให้การตอบสนองทางภาพทันทีก่อนที่คำสั่งจะถึงฮาร์ดแวร์จริง

ตัวอย่างนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
