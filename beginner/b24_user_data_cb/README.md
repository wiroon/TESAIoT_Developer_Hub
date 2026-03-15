# คอลแบ็ก User Data (User Data Callback)

ตัวอย่างนี้แสดงวิธีใช้ user data pointer กับ event callback เพื่อแยกแยะระหว่าง widget หลายตัวที่ใช้ handler เดียวกัน ปุ่มสี่ตัวแต่ละตัวพกข้อมูล struct ที่มีชื่อและสี ส่งผ่าน `lv_obj_add_event_cb`

เมื่อปุ่มใดถูกกด callback ที่ใช้ร่วมกันจะ cast `lv_event_get_user_data` เป็นประเภท struct ของแอปพลิเคชัน จากนั้นอ่าน name field เพื่ออัปเดต output label และ color field เพื่อเปลี่ยนพื้นหลังของ display panel

การใช้ struct pointer เป็น user data มีประสิทธิภาพมากกว่าการส่ง integer ธรรมดา ทำให้แต่ละ widget พกข้อมูลบริบทที่หลากหลาย: ชื่อ ID ค่าตั้งค่า หรือ pointer ไปยัง widget ที่เกี่ยวข้อง

นี่คือรูปแบบมาตรฐานสำหรับสร้าง LVGL interface ที่ scalable ซึ่ง widget หลายสิบตัวต้องการการจัดการ event รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
