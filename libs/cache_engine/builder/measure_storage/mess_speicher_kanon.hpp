#pragma once
// mess_speicher_kanon.hpp -- MeasureStorage (2026-08-09): die HAUSKONSTANTE und die ROHE
// RESERVIERUNG. Gemeinsame Unterlage der ZWEI Arenen (mess_arena.hpp, stapel_arena.hpp).
//
// SELBSTCHECK: diese Datei enthaelt KEINE Arena, KEINEN Zaehler und KEINE Ueberlauf-Politik. Sie
// haelt genau zwei Dinge: die gemessene Cacheline-Groesse und einen anonymen Seiten-Block mit
// optionaler Vorab-Beruehrung. Wer hier eine Anhaenge-Logik oder eine Fehlerklasse sucht, ist in
// der falschen Datei -- die stehen in den beiden Arenen, und zwar JE EINE EIGENE.
//
// == DIE HAUSKONSTANTE, UND WARUM SIE EINE KONSTANTE IST ============================================
// Der Deep-Research-Entscheid (Punkt 5, Owner-freigegeben 09.08.2026) verbietet ausdruecklich
// std::hardware_destructive_interference_size: sein Wert ist laut Clang-Dokumentation "not stable
// between releases" und haengt an Uebersetzungs-Flags. Bei sechs CEB-Varianten mit verschiedenen
// Flags waere das ein reales ABI-Risiko -- zwei Varianten haetten verschieden grosse Arenen-Objekte
// und derselbe Speicher-Block waere je nach Binary anders geschnitten.
//
// Die Zahl ist deshalb GEMESSEN und festgeschrieben, nicht abgeleitet. Erhebung am Objekt auf prod1
// (2026-08-09), alle vier Cache-Ebenen einzeln:
//     cat /sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size  -> 64
//     cat /sys/devices/system/cpu/cpu0/cache/index1/coherency_line_size  -> 64
//     cat /sys/devices/system/cpu/cpu0/cache/index2/coherency_line_size  -> 64
//     cat /sys/devices/system/cpu/cpu0/cache/index3/coherency_line_size  -> 64
//     getconf LEVEL1_DCACHE_LINESIZE                                     -> 64
// Nenner: 4 von 4 Cache-Ebenen plus die libc-Auskunft, fuenf Quellen, ein Wert.
//
// EINE FESTGESCHRIEBENE ZAHL DARF NICHT GEGLAUBT WERDEN: test_ms1_arenen_kein_alloc_im_fenster liest
// dieselbe sysfs-Datei zur Laufzeit und vergleicht. Weicht die Maschine ab, faellt der Test -- die
// Konstante ist damit verifiziert und nicht behauptet. Ist die Datei nicht lesbar (Container ohne
// sysfs), sagt der Test das ausdruecklich, statt still durchzulaufen.
//
// == WARUM EIN EIGENER SEITEN-BLOCK JE ARENA ========================================================
// Owner-KERN 09.08.2026: der Stapel liegt "in einer GETRENNTEN custom Arena". Woertlich getrennt
// heisst hier: ein eigener anonymer Block je Arena, nicht zwei Abschnitte eines Blocks. Das ist
// nicht Zierde -- zwei getrennte Reservierungen liegen zwangslaeufig auf verschiedenen Seiten, und
// damit ist False Sharing zwischen Nutzlast der Mess-Arena und Nutzlast des Stapels nicht bloss
// unwahrscheinlich, sondern strukturell unmoeglich. Fuer die HEISSEN ZUSTANDS-Felder (Basis, Top,
// Zaehler) leistet das die Ausrichtung der Arenen-Objekte selbst, s. checkpoint_speicher.hpp.

#include <cstddef>
#include <cstdint>
#include <cstring>

#if defined(__linux__)
#include <sys/mman.h>
#else
#include <cstdlib>
#endif

namespace comdare::cache_engine::builder::measure_storage {

// Gemessen auf prod1, s. Kopf. Verifiziert zur Laufzeit durch test_ms1_arenen_kein_alloc_im_fenster.
inline constexpr std::size_t kCacheLineBytes = 64;

// getconf PAGESIZE -> 4096 (prod1, 2026-08-09). Nur fuer die Aufrundung der Reservierung und fuer
// die Seitenfehler-Erwartung in test_ms2_pre_touch_seitenfehler; keine Layout-Bedeutung.
inline constexpr std::size_t kSeitenBytes = 4096;

/// Vorab-Beruehrung: ob die Seiten SCHON IM AUFBAU belegt werden.
///
/// Reserviert ist nicht belegt. Ein anonymer Block liefert virtuellen Adressraum; der erste
/// Schreibzugriff je Seite kostet einen Minor Fault. Ohne Vorab-Beruehrung laegen diese Fehler
/// MITTEN IM MESSFENSTER. Mit ihr wandern die Kosten in den Aufbau. Wie gross der Unterschied
/// wirklich ist, wird gemessen (test_ms2_pre_touch_seitenfehler) und nicht geglaubt.
///
/// Der Schalter ist BEWUSST ein Laufzeit-Wert und keine statische Achse: er wird ausschliesslich im
/// Aufbau gelesen (reservieren()), nie im heissen Pfad, und der Seitenfehler-Test braucht beide
/// Faelle im SELBEN Binary -- eine statische Achse machte den Vergleich unmoeglich.
enum class VorabBeruehrung : std::uint8_t { Nein = 0, Ja = 1 };

/// SeitenReservierung -- ein anonymer, privater Seiten-Block. Kein Wachstum, keine Umlagerung.
///
/// SELBSTCHECK: die Klasse hat KEIN Anhaengen und KEINEN Index. Sie besorgt Seiten und gibt sie
/// zurueck, mehr nicht. Alles Ordnende steht in den Arenen.
class SeitenReservierung {
public:
    SeitenReservierung() noexcept = default;
    ~SeitenReservierung() { freigeben(); }

    SeitenReservierung(SeitenReservierung const&)            = delete;
    SeitenReservierung& operator=(SeitenReservierung const&) = delete;

    SeitenReservierung(SeitenReservierung&& other) noexcept : basis_(other.basis_), bytes_(other.bytes_) {
        other.basis_ = nullptr;
        other.bytes_ = 0;
    }
    SeitenReservierung& operator=(SeitenReservierung&& other) noexcept {
        if (this != &other) {
            freigeben();
            basis_       = other.basis_;
            bytes_       = other.bytes_;
            other.basis_ = nullptr;
            other.bytes_ = 0;
        }
        return *this;
    }

    /// Besorgt mindestens `bytes` als privaten anonymen Block, auf ganze Seiten aufgerundet.
    /// Laeuft im AUFBAU, nie im Messfenster -- hier ist Belegung erlaubt und erwuenscht.
    /// Rueckgabe false = nicht bekommen (der Aufrufer meldet, er bricht nicht ab).
    [[nodiscard]] bool reservieren(std::size_t bytes, VorabBeruehrung vorab) noexcept {
        freigeben();
        if (bytes == 0u) return false;
        std::size_t const gerundet = (bytes + kSeitenBytes - 1u) / kSeitenBytes * kSeitenBytes;
        // RUECKRECHNUNGS-WAECHTER auf der Aufrundung. Sie ist eine Addition und kann ueberlaufen:
        // fuer bytes nahe SIZE_MAX wickelt `bytes + kSeitenBytes - 1u`, und das Ergebnis wird
        // KLEINER als die Anforderung. Die Pruefung ist exakt -- eine Aufrundung darf nie
        // schrumpfen, und nur der Ueberlauf laesst sie schrumpfen.
        //
        // WAS SIE WIRKLICH TRAEGT, ehrlich benannt (sonst waere sie eine Wache ohne Eingang): auf
        // dem LINUX-Pfad ist ihre Wirkung nicht beobachtbar. Der Ueberlauf faellt hier immer auf
        // gerundet == 0, und mmap weist eine Laenge von 0 selbst mit EINVAL ab -- reservieren()
        // liefert also so oder so false. Auf dem ERSATZWEG ist es anders: std::aligned_alloc darf
        // fuer 0 Byte einen NICHT-nullen Zeiger liefern, und dann meldete steht() einen Block, der
        // keiner ist. Diese Zeile ist damit die einzige Wache des Ersatzwegs. Sie steht hier und
        // nicht dort, weil die Rechnung beiden gemeinsam ist -- den Ersatzweg selbst kann diese
        // Maschine nicht fahren, was der Kommentar unten schon fuer VirtualAlloc feststellt.
        if (gerundet < bytes) return false;
#if defined(__linux__)
        void* p = ::mmap(nullptr, gerundet, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) return false;
#else
        // Ersatzweg fuer Nicht-Linux (die 8er-Docker-Matrix ist Linux; MSVC-Uebersetzbarkeit bleibt
        // erhalten). Die Windows-Entsprechung waere VirtualAlloc mit MEM_COMMIT -- hier NICHT gebaut,
        // weil sie auf dieser Plattform nicht geprueft werden kann und ungeprueft nichts taugt.
        void* p = std::aligned_alloc(kSeitenBytes, gerundet);
        if (p == nullptr) return false;
#endif
        basis_ = p;
        bytes_ = gerundet;
        if (vorab == VorabBeruehrung::Ja) {
            // Vorbild im Haus: custom_allocation_1_measurements.hpp:58 "Pre-touch (Pflicht: kein
            // page-fault zur Laufzeit!)". Ein Schreibzugriff je Seite genuegte; das volle Nullen ist
            // der Haus-Weg und liefert zusaetzlich einen definierten Anfangszustand.
            std::memset(basis_, 0, bytes_);
        }
        return true;
    }

    void freigeben() noexcept {
        if (basis_ == nullptr) return;
#if defined(__linux__)
        ::munmap(basis_, bytes_);
#else
        std::free(basis_);
#endif
        basis_ = nullptr;
        bytes_ = 0;
    }

    [[nodiscard]] void*       basis() const noexcept { return basis_; }
    [[nodiscard]] std::size_t bytes() const noexcept { return bytes_; }
    [[nodiscard]] bool        steht() const noexcept { return basis_ != nullptr; }

private:
    void*       basis_ = nullptr;
    std::size_t bytes_ = 0;
};

} // namespace comdare::cache_engine::builder::measure_storage
