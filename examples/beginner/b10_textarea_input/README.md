# ช่องป้อนข้อความ (Textarea Input)

ตัวอย่างนี้สาธิตการป้อนข้อความด้วย LVGL Textarea ร่วมกับ Keyboard บนหน้าจอ การแตะ Textarea จะเปิดโฟกัส และ Keyboard จะส่ง key event ไปยัง Textarea ที่ active

Keyboard widget เชื่อมต่อกับ Textarea ผ่าน `lv_keyboard_set_textarea` Keyboard จัดการ logic การป้อนทั้งหมดภายใน รวมถึงการแทรกอักขระ backspace และปุ่ม Enter Textarea รองรับทั้งโหมดบรรทัดเดียวและหลายบรรทัด

Textarea ถูกกำหนดด้วย placeholder text เพื่อแนะนำผู้ใช้ว่าควรป้อนอะไร Keyboard วางอยู่ด้านล่างของหน้าจอ ตามรูปแบบการป้อนข้อมูลมาตรฐานของมือถือ

รูปแบบการป้อนข้อความนี้จำเป็นสำหรับการป้อนรหัสผ่าน WiFi การตั้งชื่ออุปกรณ์ และการตั้งค่าอื่นๆ รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
