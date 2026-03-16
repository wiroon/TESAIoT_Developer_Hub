# พื้นหลังหน้าจอ (Screen Background)

ตัวอย่างนี้สาธิตวิธีตั้งพื้นหลังแบบ gradient แนวตั้งบน parent container gradient ไล่จากสีน้ำเงินเข้มด้านบนไปสีน้ำเงินอ่อนด้านล่าง สร้างลักษณะแอปที่ทันสมัย

gradient ถูกกำหนดด้วย style property สามตัว: `bg_color` สำหรับสีเริ่มต้น `bg_grad_color` สำหรับสีสิ้นสุด และ `bg_grad_dir` ตั้งเป็น `LV_GRAD_DIR_VER` สำหรับทิศทางแนวตั้ง

label จัดกลางด้วยข้อความสีขาวสาธิตเนื้อหาที่วางบนพื้นหลัง gradient label ใช้ panel พื้นหลังกึ่งโปร่งใสเพื่อรับรองความสามารถในการอ่านบน gradient

พื้นหลัง gradient เพิ่มความสวยงามให้ IoT dashboard และหน้าจอเริ่มต้น เทคนิคนี้รองรับทั้ง TESAIoT Dev Kit, PSoC Edge AI Kit และ PSoC Edge Eva Kit
