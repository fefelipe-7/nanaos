# nanaos

## Fase 1: Boot funcional

Este repositório contém a base inicial para `nanaos`, com um kernel mínimo que roda em QEMU via GRUB.

### Estrutura inicial

- `boot/grub/grub.cfg` — configuração do GRUB para carregar o kernel
- `kernel/boot.s` — entrada em assembly e cabeçalho Multiboot2
- `kernel/core/kernel.c` — núcleo modular `nanacore` em fase 2
- `kernel/linker.ld` — linker script para o kernel freestanding
- `Makefile` — compilação e geração de ISO bootável

### Estrutura de Fase 2

- `kernel/arch/x86_64/gdt/` — GDT para x86_64
- `kernel/arch/x86_64/idt/` — IDT e stubs de interrupção
- `kernel/arch/x86_64/interrupts/` — PIC e registro de handlers
- `kernel/arch/x86_64/port/` — operações `inb`/`outb`
- `kernel/drivers/serial/` — driver COM1
- `kernel/drivers/timer/` — driver PIT e tick counter
- `kernel/drivers/keyboard/` — PS/2 keyboard
- `kernel/terminal/` — VGA terminal e escrita de texto
- `kernel/panic/` — panic handler
- `kernel/shell/` — shell de linha de comando
- `kernel/memory/` — comandos `meminfo` e `uptime`
- `kernel/lib/` — utilitários de string e conversão

### Pré-requisitos

No Linux, instale:

- `gcc` (compilador C)
- `binutils` (`ld`)
- `grub-mkrescue` ou `xorriso`
- `qemu-system-x86_64`

### Como construir

```sh
make
```

### Como executar

```sh
make run
```

### Fluxo de boot

1. QEMU carrega o CD-ROM ISO criado em `build/nanaos.iso`.
2. GRUB inicializa e lê `boot/grub/grub.cfg`.
3. GRUB carrega `boot/kernel.elf` como kernel Multiboot2.
4. O kernel entra em `start` em `kernel/boot.s`.
5. O kernel inicializa a pilha e chama `kernel_main` em `kernel/main.c`.
6. `kernel_main` escreve texto diretamente no framebuffer VGA em `0xB8000`.

### Explicação dos arquivos

- `kernel/boot.s` contém o cabeçalho Multiboot2 exigido pelo GRUB, troca para um stack seguro e desativa interrupções antes de entrar no kernel.
- `kernel/main.c` é um kernel freestanding; ele não usa a biblioteca padrão e acessa hardware diretamente.
- `kernel/linker.ld` posiciona o kernel em memória no endereço físico `0x00100000`, onde kernels x86_64 costumam ser carregados.
- `Makefile` orquestra a compilação, o linking e a criação da imagem ISO.

### Observações

Nesta fase 1, o kernel não faz gerenciamento de memória ou drivers além do texto VGA. O objetivo é validar a rota `GRUB → kernel → execução`.
