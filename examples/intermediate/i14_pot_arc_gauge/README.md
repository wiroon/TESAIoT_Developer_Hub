# Arc Gauge ตัวต้านทานปรับค่า (Potentiometer Arc Gauge)

ตัวอย่างนี้ map ค่าอ่าน potentiometer ของ Eva Kit ไปยัง arc gauge ขนาดใหญ่แสดง 0-100% สีตัวบ่งชี้ arc เปลี่ยนตามค่า: เขียวเมื่อต่ำกว่า 50% เหลืองระหว่าง 50-80% และแดงเมื่อเกิน 80% ให้ผลตอบรับภาพที่เข้าใจง่าย

ค่า ADC ของ potentiometer ถูกอ่านทุก 50ms และสเกลเป็นเปอร์เซ็นต์ 0-100 arc gauge ใช้การกวาด 270 องศา (จาก 135 ถึง 45 องศา) เป็นรูปลักษณ์ gauge คลาสสิก label ตรงกลางขนาดใหญ่แสดงค่าเปอร์เซ็นต์ และ label เล็กด้านล่างแสดงค่า ADC ดิบ

gradient สีถูกทำโดยเปลี่ยนสีตัวบ่งชี้ arc ใน timer callback ตามเกณฑ์ที่กำหนด วิธีง่ายนี้หลีกเลี่ยงความซับซ้อนของ gradient rendering แต่ยังคงสื่อสารขนาดค่าผ่านสีได้

ตัวอย่างนี้ใช้ได้เฉพาะ Eva Kit เนื่องจาก AI Kit ไม่มี potentiometer guard `#if BSP_HAS_POTENTIOMETER` ทำให้คอมไพล์ได้สะอาดบนบอร์ดอื่น รูปแบบการ map analog input ไปยัง visual gauge เป็นพื้นฐานสำหรับแอปพลิเคชัน industrial HMI

รองรับ TESAIoT Dev Kit และ PSoC Edge Eva Kit
