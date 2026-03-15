# ตัวเลือก Checkbox (Checkbox Options)

ตัวอย่างนี้สร้าง Checkbox widget สี่ตัวแทนตัวเลือกเซ็นเซอร์: Accelerometer, Gyroscope, Temperature และ Pressure โดย Label สรุปด้านล่างอัปเดตแบบ dynamic เพื่อแสดงเซ็นเซอร์ที่ถูกเลือกอยู่

แต่ละ Checkbox ส่ง `LV_EVENT_VALUE_CHANGED` เมื่อถูกสลับ callback ที่ใช้ร่วมกันจะวนตรวจสอบ Checkbox ทั้งสี่ตัว และสร้างข้อความสรุปตามตัวที่มี `LV_STATE_CHECKED` สาธิตวิธีจัดการ control ที่เกี่ยวข้องกันหลายตัวด้วย callback function เดียว

Checkbox ถูกจัดเรียงแนวตั้งด้วย flex column layout พร้อมระยะห่างที่เหมาะสม Label สรุปใช้ข้อความ formatted เพื่อรวมชื่อรายการที่เลือกทั้งหมด ให้การตอบสนองทันทีแก่ผู้ใช้

ตัวอย่างนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit เนื่องจากใช้เพียง LVGL widget
