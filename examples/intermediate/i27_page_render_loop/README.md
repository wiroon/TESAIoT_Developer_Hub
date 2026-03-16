# ลูป Render ของหน้า (Page Render Loop)

ตัวอย่างนี้สาธิตรูปแบบ page render callback ที่ใช้โดย dashboard จริง LVGL timer จำลองการเรียก render แบบคาบของ page manager อัปเดตค่าเซ็นเซอร์ ตัวนับเฟรม และกราฟทุกรอบ tick

ใน firmware จริง page manager เรียกฟังก์ชัน `render` ของหน้าที่ active ในช่วงเวลาคงที่ (ผูกกับ refresh period ของ LVGL) ตัวอย่างนี้จำลองพฤติกรรมนั้นด้วย LVGL timer 100ms ที่อ่าน BMI270 และอัปเดต UI element หลายตัว

ฟังก์ชัน render อัปเดตกราฟด้วยข้อมูล accelerometer ใหม่ รีเฟรช label ค่า 3 ตัว เพิ่มตัวนับเฟรม และคำนวณ FPS โดยประมาณ การอัปเดตทั้งหมดเกิดขึ้นภายใน render call เดียว ทำให้จอแสดงผลสอดคล้องกันในแต่ละเฟรม

หลักการสำคัญที่สาธิตที่นี่คือ render callback ควรเร็วและไม่บล็อก ควรอ่านจากข้อมูลเซ็นเซอร์ที่ cache ไว้ล่วงหน้า (หรือ volatile buffer ที่เติมโดย acquisition task แยกต่างหาก) แทนที่จะทำ I/O ช้าโดยตรง ทำให้ GFX task ตอบสนองไว

รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
