# LostDreamVST

Аудио плагин (VST3/AU) на фреймворке JUCE.

## Компиляция на macOS

### 1. Необходимое ПО

**Xcode**
- Скачайть Xcode из Mac App Store
- В терминале выполнить:
```bash
sudo xcode-select --install
sudo xcodebuild -license accept
```
**JUCE**

-Скачайть с официального сайта: https://juce.com/get-juce


### 2. Клонирование репозитория
```bash
git clone https://github.com/o13scure/LostDreamVST.git
cd LostDreamVST
```
ну или можно просто скачать файлы

### 3. Откройте проект в Projucer
Запустите Projucer (находится в папке JUCE)

Нажмите Open Project... и выберите файл LostDream.jucer

### 4. Сгенерируйте Xcode проект
В Projucer:

Выберите Save Project and Open in IDE (или нажмите Cmd+S)

Или: Exporters → выберите Xcode (macOS) → Save

### 5. Скомпилируйте плагин
Через Xcode:

-Откроется Xcode проект

-Выберите схему: LostDream - VST3 или LostDream - AU

-Нажмите Cmd+B для сборки

**Готовые плагины появятся в:**

-VST3: ~/Library/Audio/Plug-Ins/VST3/

-Audio Unit: ~/Library/Audio/Plug-Ins/Components/

