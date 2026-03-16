# ลำดับ LED (LED Sequence)

ตัวอย่างนี้สร้างลำดับ LED สไตล์ Knight Rider ที่กระเด้งระหว่าง LED สามตัว: Red, Green และ Blue ลำดับทำงาน R, G, B, G, R และวนซ้ำอย่างต่อเนื่อง ขับเคลื่อนด้วย LVGL timer state machine

ตัวแปรสถานะติดตาม LED ปัจจุบันในลำดับ ทุกรอบ timer (200ms) LED ปัจจุบันจะปิดและ LED ถัดไปในลำดับจะเปิด ทิศทางกลับที่ปลายแต่ละด้าน สร้างเอฟเฟกต์กระเด้งที่เป็นเอกลักษณ์

ทั้ง LED ฮาร์ดแวร์ (ผ่าน GPIO) และ Virtual LVGL LED ถูกควบคุมพร้อมกัน UI แสดงตัวบ่งชี้ LED ขนาดใหญ่สามตัวเรียงกัน ตัวที่ active สว่างและตัวอื่นหรี่

ปุ่มควบคุมความเร็วให้ผู้ใช้เปลี่ยนความเร็ว animation จาก 50ms ถึง 500ms สาธิตการเปลี่ยน timer period แบบ dynamic รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
