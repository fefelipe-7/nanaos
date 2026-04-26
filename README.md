<div align="center">

<img src="https://via.placeholder.com/800x200?text=nanaos+os" alt="nanaos banner"/>

<h1>nanaos</h1>

<p>
  um sistema operacional experimental construído do zero<br>
  focado em controle total, entendimento profundo e evolução progressiva
</p>

<p>
  <img src="https://img.shields.io/badge/status-em%20desenvolvimento-yellow"/>
  <img src="https://img.shields.io/badge/arch-x86__64-blue"/>
  <img src="https://img.shields.io/badge/kernel-freestanding-lightgrey"/>
  <img src="https://img.shields.io/badge/language-c%20%7C%20asm-black"/>
  <img src="https://img.shields.io/badge/build-make-green"/>
</p>

</div>

---

# 📚 conteúdo

- [visão geral](#-visão-geral)
- [why nanaos](#-why-nanaos)
- [philosophy](#-philosophy)
- [estrutura](#-estrutura)
- [como rodar](#-como-rodar)
- [fluxo de boot](#-fluxo-de-boot)
- [arquitetura](#-arquitetura)
- [status](#-status)
- [roadmap](#-roadmap)

---

# 🧩 visão geral

o **nanaos** é um sistema operacional criado do zero com o objetivo de explorar profundamente:

- como um sistema realmente funciona internamente  
- como o hardware é controlado diretamente  
- como abstrações são construídas do nível mais baixo possível  

não é apenas sobre construir um os —  
é sobre **entender cada camada que compõe um sistema moderno**.

---

# 🚀 why nanaos

existem muitos sistemas operacionais prontos.  
então por que construir outro?

porque usar um sistema ≠ entender um sistema.

o nanaos existe para:

- remover abstrações desnecessárias  
- expor o funcionamento real do sistema  
- permitir controle total sobre cada componente  
- servir como laboratório de sistemas  

> "se você não construiu, você não entende completamente"

---

# 🧠 philosophy

o projeto segue alguns princípios fundamentais:

### minimalismo
tudo começa simples. complexidade só é adicionada quando necessária.

### controle total
nenhuma dependência externa desnecessária. o sistema é totalmente freestanding.

### aprendizado first
decisões são guiadas por aprendizado profundo, não apenas conveniência.

### evolução por camadas
cada fase adiciona uma nova camada de abstração sobre a anterior:

```
boot → kernel → memória → drivers → syscalls → interface
```

---

# 📦 estrutura

## 🧱 base do sistema

```
boot/
 └── grub/grub.cfg

kernel/
 ├── boot.s
 ├── core/kernel.c
 ├── linker.ld

makefile
```

## ⚙️ arquitetura (fase 2)

```
kernel/arch/x86_64/
 ├── gdt/
 ├── idt/
 ├── interrupts/
 ├── port/

kernel/drivers/
 ├── serial/
 ├── timer/
 ├── keyboard/

kernel/
 ├── terminal/
 ├── panic/
 ├── shell/
 ├── memory/
 ├── lib/
```

---

# 🛠️ como rodar

## pré-requisitos

- gcc  
- binutils  
- grub-mkrescue ou xorriso  
- qemu-system-x86_64  

## build

```sh
make
```

## execução

```sh
make run
```

---

# 🔄 fluxo de boot

```
qemu → grub → kernel → execução
```

1. qemu carrega a iso (`build/nanaos.iso`)  
2. grub inicializa e lê `grub.cfg`  
3. kernel é carregado via multiboot2  
4. entrada em `boot.s`  
5. inicialização da stack  
6. chamada de `kernel_main`  
7. escrita direta no vga (`0xb8000`)  

---

# ⚙️ arquitetura

## componentes principais

### boot
- entrada em assembly
- header multiboot2
- setup inicial do ambiente

### kernel
- núcleo freestanding
- sem libc
- controle direto de memória e hardware

### drivers
- comunicação direta com dispositivos
- serial, timer, teclado

### sistema base
- terminal vga
- shell simples
- utilitários internos

---

# 📊 status

| fase | descrição           | status |
|------|--------------------|--------|
| 1    | boot funcional     | ✅ concluído |
| 2    | base arquitetural  | 🚧 em progresso |

---

# 🗺️ roadmap

### core system
- fase 3 → gerenciamento de memória  
- fase 4 → drivers avançados  
- fase 5 → filesystem  

### kernel evolution
- fase 6 → syscalls  
- fase 7 → multitasking  
- fase 8 → isolamento de processos  

### interface
- fase futura → framebuffer gráfico  
- window manager  
- ambiente completo  

---

# ⚠️ observações

estado atual do sistema:

- sem gerenciamento de memória avançado  
- drivers mínimos  
- saída apenas via vga  

objetivo atual:

> validar completamente o pipeline: boot → execução

---

# 🔭 futuro do projeto

o objetivo final é evoluir o nanaos para um sistema completo com:

- interface gráfica própria  
- gerenciamento de janelas  
- sistema modular  
- ecossistema próprio  

---

<p align="center">
  construindo entendimento real de sistemas, uma camada por vez
</p>
