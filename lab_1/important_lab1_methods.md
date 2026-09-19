# Important lab 1 methods

## fork

New process via duplicating.
- New is child, method is

Return value
- PID av child ges i parent
- 0 i child
- -1 i parent om ingen child, errno sätts för indicate error

## exec

Replaces current process with new

Första argument alltid handlar om filens namn som kommer köras.

Return sker bara om något gick fel, då är det -1 och kan se errno för fel

Bokstäver nedan

### l

list of one or more pointers to null-terminated strings
- Första ska point till filename som associeras med filen som ska exekveras.
- List of arguments måste bli terminated via en null pointer

Jämfört med l, så specificerar v command-line arguments av executed program som en vector.

### v

array pointers null-terminated strings som representera argument list som finns till nytt program
- Första ska point till filename som associeras med filen som ska exekveras

### e

Environment via envp
- Envp är array of pointers till null-terminated strings som måste bli terminated via en null pointer

De utan e, tar environment via extern environ

### p

Om inga slashes då börjar kolla via path?
- De **utan** p tar första argument som en relative eller absolute pathname

## wait

(egen header)

Väntar på att child ändrar state, dessa kan vara:
- Child terminate
    - När det gäller denna, en wait fixar så system kan release resources
    - Utan wait kan det blir zombie
- Child stopped via signal
- Child resumed via signal

Om redan ändrat state returnerar direkt
- Annars block tills child ändrar state eller signal handler interrupt call

En child som ändrat state, men ej blivit väntad på är waitable

wait är samma som waitpid(-1, &wstatus, 0);

waitpid parametrar:
- pid för specificera child pid
    - mindre än -1, vänta för någon child process som process group ID är samma som absolute value of pid given
    - -1 för att vänta på någon child process
    - 0 för vänta på någon child process som har samma process group ID som calling process när waitpid() var called
    - större än 0 vänta på child process ID som är samma som pid given
- wstatus
    - Denna är null som man inte ska använda linux-specifika options från vad jag fattar
        - Om ej null kommer spara status info till int som den kommer peka till
- Options
    - Är en OR operation av noll eller fler konstanter
        - Ex: WHOHANG, WUNTRACED, WCONTINUED

## stat

Läsa info om filen
- Behöver inte permissions på själva filen, men directories som leder till den

Olika versioner:
- `stat` få info via path
- `lstat` har symbolic link
- `fstat` har file descriptor
- `fstatat` mer general som kan ha exakt samma beteende som de andra

Returnerar 0 om lyckas, -1 om error, errno för indicate error.

## signal

(egen header)

signal beteende kan variera, sigaction bättre?

Signal sätter disposition? av signal *signum* till handler kommer vara
- SIG_IGN
    - IGN står för ignore
- SIG_DFL
    - DFL står för default, gör default action som associeras med signal
- adress av programer-defined function som då vara en signal handler
    - Om vara funktion, antingen reset till SIG_DFL eller signal blocked, sedan kommer handler bli kallad med argument *signum*. Om invocation av handler göra att handler bli blocked då kommer signal bli unblocked på return från handler.

Returnerar förra värde av signal handlar, om fail, då SIG_ERR och errno för att se error

## pipe

Används för att skapa unidirectional data channel för interprocess communication.

pipefd för file descriptors för ends of pipe
- 0 read end
- 1 write end

Data som skrivs till write är buffered av kernel till det läses från read end

pipe2 har flaggor som kan sättas, om 0 samma som pipe men kan anpassa via bitwise ORs
- Ex: O_CLOEXEC, O_DIRECT, O_NONBLOCK, O_NOTIFICATION_PIPE

Returnerar 0 om lyckas, -1 om fail, errno för se fel

## dup

duplicate a file descriptor

Finns:
- dup
    - Allokerar en ny file descriptor som refererar till samma öppna file descriptor som oldfd
        - Kan kolla open file descriptions via open man page
    - Nya file descriptor kommer vara lowest-numbered file descriptor som är unused i calling process
    - Vid successful return, gammal och ny kan användas interchangeable, även file status flags är synkade, men delar inte file descriptor flags
- dup2
    - Samma som dup(), men istället för lowest-numbered unused, nu använda file descriptor som specificeras i newfd. Alltså new blir adjusted så att referera till samma som old
- dup3
    - Kan force close-on-exec flag

Returnerar
- Success --> new file descriptor
- Error --> -1, kolla errno för se fel

## errno

(egen header)

har siffror, antar att hur dessa ska tolkas kolla man pages, för errno säger nog mer allmänt hur de funka
