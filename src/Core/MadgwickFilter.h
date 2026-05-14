#pragma once
#include <math.h>

/**
 * Filtre Madgwick AHRS
 * Fusionne accel + gyro pour produire un quaternion d'orientation stable.
 * Optimisé pour la FPU de l'ESP32-S3 (opérations float 32-bit).
 * 
 * Ref: Madgwick, S. (2010). An efficient orientation filter for inertial
 *      and inertial/magnetic sensor arrays.
 */
class MadgwickFilter {
public:
    // beta : gain de correction accel sur gyro
    // Valeurs typiques : 0.033 (stable/lent) à 0.1 (réactif/snowboard)
    // Plus grand = plus de correction accel, moins de drift gyro, mais plus de bruit
    explicit MadgwickFilter(float beta = 0.05f)
        : _beta(beta), _beta_base(beta),
          _adaptive(false),
          _q0(1.f), _q1(0.f), _q2(0.f), _q3(0.f) {}

    void setBeta(float beta) { _beta = beta; _beta_base = beta; }

    /**
     * Mode beta adaptatif : en virage (|gz| élevé) on baisse beta pour faire
     * davantage confiance au gyro et ignorer l'accéléro bruité par les
     * vibrations / forces centripètes. En straight, on remonte beta pour
     * recaler l'orientation sur la gravité.
     *
     * @param low_beta   ex. 0.02 - utilisé quand |gz| > high_omega_dps
     * @param high_beta  ex. 0.10 - utilisé quand |gz| < low_omega_dps
     * @param low_omega_dps   borne basse (deg/s) — ex. 5
     * @param high_omega_dps  borne haute (deg/s) — ex. 30
     * Entre les deux on interpole linéairement.
     */
    void enableAdaptiveBeta(float low_beta, float high_beta,
                            float low_omega_dps, float high_omega_dps) {
        _adaptive = true;
        _beta_lo  = low_beta;
        _beta_hi  = high_beta;
        _omega_lo = low_omega_dps;
        _omega_hi = high_omega_dps;
    }

    void disableAdaptiveBeta() { _adaptive = false; _beta = _beta_base; }

    /** Recalcule _beta selon |omega| courant si adaptatif activé. */
    void _adaptBetaIfNeeded(float gx, float gy, float gz) {
        if (!_adaptive) return;
        // Magnitude angulaire totale (deg/s). Note : on travaille en deg/s
        // au moment de l'appel, pas en rad/s (cf. update() qui convertit ensuite).
        const float omega = sqrtf(gx*gx + gy*gy + gz*gz);
        if (omega >= _omega_hi) {
            _beta = _beta_lo;
        } else if (omega <= _omega_lo) {
            _beta = _beta_hi;
        } else {
            const float t = (omega - _omega_lo) / (_omega_hi - _omega_lo);
            _beta = _beta_hi + t * (_beta_lo - _beta_hi);
        }
    }

    // Mise à jour principale — appeler à chaque sample IMU
    // gx/gy/gz : degrés/seconde  |  ax/ay/az : g (non normalisé)
    void update(float gx, float gy, float gz,
                float ax, float ay, float az, float dt) {

        // Adapter beta selon la magnitude angulaire AVANT conversion rad/s.
        _adaptBetaIfNeeded(gx, gy, gz);

        // Gyro en rad/s
        gx *= DEG_TO_RAD;
        gy *= DEG_TO_RAD;
        gz *= DEG_TO_RAD;

        float q0 = _q0, q1 = _q1, q2 = _q2, q3 = _q3;

        // Normalise l'accéléromètre — si le vecteur est nul, on skip la correction
        float norm = sqrtf(ax*ax + ay*ay + az*az);
        if (norm < 1e-6f) {
            // Intégration gyro seule
            _integrateGyroOnly(gx, gy, gz, dt);
            return;
        }
        norm = 1.f / norm;
        ax *= norm; ay *= norm; az *= norm;

        // Gradient descent — dérivée de la fonction objectif f = g(q) - a_mesure
        // g(q) = direction de la gravité dans le repère capteur selon le quaternion
        float _2q0 = 2.f * q0, _2q1 = 2.f * q1, _2q2 = 2.f * q2, _2q3 = 2.f * q3;
        float _4q0 = 4.f * q0, _4q1 = 4.f * q1, _4q2 = 4.f * q2;
        float _8q1 = 8.f * q1, _8q2 = 8.f * q2;
        float q0q0 = q0*q0, q1q1 = q1*q1, q2q2 = q2*q2, q3q3 = q3*q3;

        float s0 = _4q0*q2q2 + _2q2*ax + _4q0*q1q1 - _2q1*ay;
        float s1 = _4q1*q3q3 - _2q3*ax + 4.f*q0q0*q1 - _2q0*ay - _4q1 + _8q1*q1q1 + _8q1*q2q2 + _4q1*az;
        float s2 = 4.f*q0q0*q2 + _2q0*ax + _4q2*q3q3 - _2q3*ay - _4q2 + _8q2*q1q1 + _8q2*q2q2 + _4q2*az;
        float s3 = 4.f*q1q1*q3 - _2q1*ax + 4.f*q2q2*q3 - _2q2*ay;

        // Normalise le gradient
        norm = 1.f / sqrtf(s0*s0 + s1*s1 + s2*s2 + s3*s3);
        s0 *= norm; s1 *= norm; s2 *= norm; s3 *= norm;

        // Dérivée du quaternion = intégration gyro - correction gradient
        float qDot0 = 0.5f * (-q1*gx - q2*gy - q3*gz) - _beta * s0;
        float qDot1 = 0.5f * ( q0*gx + q2*gz - q3*gy) - _beta * s1;
        float qDot2 = 0.5f * ( q0*gy - q1*gz + q3*gx) - _beta * s2;
        float qDot3 = 0.5f * ( q0*gz + q1*gy - q2*gx) - _beta * s3;

        // Intégration Euler
        q0 += qDot0 * dt;
        q1 += qDot1 * dt;
        q2 += qDot2 * dt;
        q3 += qDot3 * dt;

        // Re-normalise le quaternion
        norm = 1.f / sqrtf(q0*q0 + q1*q1 + q2*q2 + q3*q3);
        _q0 = q0*norm; _q1 = q1*norm; _q2 = q2*norm; _q3 = q3*norm;
    }

    // --- Angles d'Euler extraits du quaternion ---

    // Roll : rotation autour de l'axe X (inclinaison latérale de la planche)
    float getRoll() const {
        return atan2f(2.f*(_q0*_q1 + _q2*_q3),
                      1.f - 2.f*(_q1*_q1 + _q2*_q2)) * RAD_TO_DEG;
    }

    // Pitch : rotation autour de l'axe Y (inclinaison avant/arrière)
    float getPitch() const {
        float sinp = 2.f*(_q0*_q2 - _q3*_q1);
        sinp = sinp >  1.f ?  1.f : sinp;
        sinp = sinp < -1.f ? -1.f : sinp;
        return asinf(sinp) * RAD_TO_DEG;
    }

    // Yaw : cap (drift sans magnéto — utile pour détecter les rotations de virage)
    float getYaw() const {
        return atan2f(2.f*(_q0*_q3 + _q1*_q2),
                      1.f - 2.f*(_q2*_q2 + _q3*_q3)) * RAD_TO_DEG;
    }

    // Torsion entre deux filtres : angle entre leurs axes X dans le plan YZ
    // Utilisé pour mesurer le twist talon→orteil sur la même fixation
    static float torsionBetween(const MadgwickFilter& a, const MadgwickFilter& b) {
        // Différence de roll entre les deux IMU — représente la flexion physique
        return a.getRoll() - b.getRoll();
    }

    void reset() { _q0=1.f; _q1=0.f; _q2=0.f; _q3=0.f; }

    // Accès brut au quaternion si besoin
    void getQuaternion(float& q0, float& q1, float& q2, float& q3) const {
        q0=_q0; q1=_q1; q2=_q2; q3=_q3;
    }

private:
    //static constexpr float DEG_TO_RAD = 0.017453292519943f;
    //static constexpr float RAD_TO_DEG = 57.295779513082f;

    float _beta;        // beta courant (peut être adapté chaque update)
    float _beta_base;   // beta défini par l'utilisateur via setBeta / ctor
    bool  _adaptive;
    float _beta_lo = 0.02f, _beta_hi = 0.10f;
    float _omega_lo = 5.f,  _omega_hi = 30.f;
    float _q0, _q1, _q2, _q3;

    void _integrateGyroOnly(float gx, float gy, float gz, float dt) {
        float q0=_q0, q1=_q1, q2=_q2, q3=_q3;
        float halfDt = 0.5f * dt;
        q0 += (-q1*gx - q2*gy - q3*gz) * halfDt;
        q1 += ( q0*gx + q2*gz - q3*gy) * halfDt;
        q2 += ( q0*gy - q1*gz + q3*gx) * halfDt;
        q3 += ( q0*gz + q1*gy - q2*gx) * halfDt;
        float norm = 1.f / sqrtf(q0*q0+q1*q1+q2*q2+q3*q3);
        _q0=q0*norm; _q1=q1*norm; _q2=q2*norm; _q3=q3*norm;
    }
};
