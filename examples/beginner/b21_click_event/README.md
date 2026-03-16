# รายละเอียด Click Event (Click Event Details)

ตัวอย่างนี้ให้มุมมองเชิงลึกของระบบ event callback ใน LVGL สร้างปุ่มสามตัว แต่ละตัวใช้ callback function เดียวกัน callback ใช้ `lv_event_get_code`, `lv_event_get_target` และ `lv_event_get_user_data` เพื่อระบุว่าเกิดอะไรขึ้นและปุ่มไหนถูกกด

แต่ละปุ่มส่งข้อความ string ที่แตกต่างเป็น user data ไปยัง callback เมื่อถูกกด callback จะแสดงชื่อปุ่มและประเภท event ใน output label สาธิตรูปแบบที่แนะนำสำหรับจัดการ widget หลายตัวด้วย callback function เดียว

event target pointer ระบุ widget เฉพาะที่ trigger event ในขณะที่ user data ให้บริบทเฉพาะแอปพลิเคชัน ทั้งสองรวมกันเปิดการจัดการ event ที่สะอาดและ scalable โดยไม่ต้องใช้ callback function แยกสำหรับแต่ละ widget

รูปแบบนี้เป็นพื้นฐานสำหรับตัวอย่าง event ทั้งหมดที่ตามมา รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
