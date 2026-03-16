# ความสว่าง LED ด้วย PWM (LED PWM Brightness)

ตัวอย่างนี้ใช้ PWM (Pulse Width Modulation) เพื่อควบคุมความสว่างของ LED ฮาร์ดแวร์ Slider บนหน้าจอปรับ duty cycle จาก 0% ถึง 100% หรี่หรือเพิ่มความสว่าง LED อย่างนุ่มนวล

PWM peripheral ถูกเริ่มต้นด้วย `cyhal_pwm_init` ที่ความถี่ 10kHz บนขา P13_7 ฟังก์ชัน `cyhal_pwm_set_duty_cycle` รับค่า float เปอร์เซ็นต์ (0.0 ถึง 100.0) ที่กำหนดว่า LED เปิดนานเท่าใดในแต่ละคาบ PWM

Virtual LED บนหน้าจอสะท้อนระดับความสว่างด้วย `lv_led_set_brightness` ให้ feedback ทางภาพแม้เมื่อมองไม่เห็น LED จริง ค่าเปอร์เซ็นต์แสดงใน label ด้านล่าง Slider

PWM จำเป็นสำหรับการควบคุมแบบ analog-like ของขาดิจิทัล แอปพลิเคชันรวมถึงความสว่าง LED ความเร็วมอเตอร์ และการสร้างเสียง buzzer รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
