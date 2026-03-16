# สไตล์กำหนดเอง (Custom Style)

ตัวอย่างนี้สาธิตการสร้าง LVGL style ที่ใช้ซ้ำได้ด้วย `lv_style_t` และนำไปใช้กับ widget หลายตัว กำหนด style สองรูปแบบ: primary style และ secondary style แต่ละตัวมีสี ขอบ และเงาที่แตกต่างกัน

Style ถูกเริ่มต้นด้วย `lv_style_init` และกำหนดค่าผ่านฟังก์ชัน setter เช่น `lv_style_set_bg_color`, `lv_style_set_radius` และ `lv_style_set_shadow_width` เมื่อกำหนดแล้ว style จะถูกนำไปใช้กับ widget ด้วย `lv_obj_add_style`

ข้อดีของ style ที่ใช้ซ้ำได้เมื่อเทียบกับ inline styling คือความสม่ำเสมอและการบำรุงรักษา การเปลี่ยน style definition จะอัปเดต widget ทั้งหมดที่ใช้ style นั้นโดยอัตโนมัติ รูปแบบนี้จำเป็นสำหรับสร้างธีมที่สอดคล้องกันในแอปพลิเคชันขนาดใหญ่

ปุ่มทั้งสองสาธิตสถานะ hover โดยเพิ่ม pressed style variant รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
