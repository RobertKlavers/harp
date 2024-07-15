mass mixing ratio derivations
=============================

   .. _derivation_mass_mixing_ratio_from_mass_density:

#. mass mixing ratio from mass density

   ================ ==================================== ====================== =================================
   symbol           description                          unit                   variable name
   ================ ==================================== ====================== =================================
   :math:`\rho`     mass density of total air            :math:`\frac{kg}{m^3}` `density {:}`
   :math:`\rho_{x}` mass density of air component x      :math:`\frac{kg}{m^3}` `<species>_density {:}`
                    (e.g. :math:`\rho_{O_{3}}`)
   :math:`q_{x}`    mass mixing ratio of air component x :math:`\frac{kg}{kg}`  `<species>_mass_mixing_ratio {:}`
                    with regard to total air
   ================ ==================================== ====================== =================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{x} = \frac{\rho_{x}}{\rho}


   .. _derivation_mass_mixing_ratio_from_volume_mixing_ratio:

#. mass mixing ratio from volume mixing ratio

   =============== ================================= ===================== ===================================
   symbol          description                       unit                  variable name
   =============== ================================= ===================== ===================================
   :math:`M_{air}` molar mass of total air           :math:`\frac{g}{mol}`    `molar_mass {:}`
   :math:`M_{x}`   molar mass of air component x     :math:`\frac{g}{mol}`
   :math:`q_{x}`   mass mixing ratio of quantity x   :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio {:}`
                   with regard to total air
   :math:`\nu_{x}` volume mixing ratio of quantity x :math:`ppv`           `<species>_volume_mixing_ratio {:}`
                   with regard to total air
   =============== ================================= ===================== ===================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{x} = \nu_{x}\frac{M_{x}}{M_{air}}


   .. _derivation_mass_mixing_ratio_from_mass_mixing_ratio_dry_air:

#. mass mixing ratio from mass mixing ratio dry air

   ==================== ==================================== ===================== =========================================
   symbol               description                          unit                  variable name
   ==================== ==================================== ===================== =========================================
   :math:`q_{x}`        mass mixing ratio of air component x :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio {:}`
                        with regard to total air
   :math:`q_{dry\_air}` mass mixing ratio of dry air with    :math:`\frac{kg}{kg}` `dry_air_mass_mixing_ratio {:}`
                        regard to total air
   :math:`\bar{q}_{x}`  mass mixing ratio of air component x :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio_dry_air {:}`
                        with regard to dry air
   ==================== ==================================== ===================== =========================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{x} = \bar{q}_{x}q_{dry\_air}


   .. _derivation_mass_mixing_ratio_dry_air_from_mass_density:

#. mass mixing ratio dry air from mass density

   ======================= ==================================== ====================== =========================================
   symbol                  description                          unit                   variable name
   ======================= ==================================== ====================== =========================================
   :math:`\rho_{dry\_air}` mass density of dry air              :math:`\frac{kg}{m^3}` `dry_air_density {:}`
   :math:`\rho_{x}`        mass density of air component x      :math:`\frac{kg}{m^3}` `<species>_density {:}`
                           (e.g. :math:`\rho_{O_{3}}`)
   :math:`\bar{q}_{x}`     mass mixing ratio if air component x :math:`\frac{kg}{kg}`  `<species>_mass_mixing_ratio_dry_air {:}`
                           with regard to dry air
   ======================= ==================================== ====================== =========================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{x} = \frac{\rho_{x}}{\rho_{dry\_air}}


   .. _derivation_mass_mixing_ratio_dry_air_from_volume_mixing_ratio_dry_air:

#. mass mixing ratio dry air from volume mixing ratio dry air

   ===================== ================================= ===================== ===========================================
   symbol                description                       unit                  variable name
   ===================== ================================= ===================== ===========================================
   :math:`M_{dry\_air}`  molar mass of dry air             :math:`\frac{g}{mol}`
   :math:`M_{x}`         molar mass of air component x     :math:`\frac{g}{mol}`
   :math:`\bar{q}_{x}`   mass mixing ratio of quantity x   :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio_dry_air {:}`
                         with regard to dry air
   :math:`\bar{\nu}_{x}` volume mixing ratio of quantity x :math:`ppv`           `<species>_volume_mixing_ratio_dry_air {:}`
                         with regard to dry air
   ===================== ================================= ===================== ===========================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      \bar{q}_{x} = \bar{\nu}_{x}\frac{M_{x}}{M_{dry\_air}}


   .. _derivation_mass_mixing_ratio_dry_air_from_mass_mixing_ratio:

#. mass mixing ratio dry air from mass mixing ratio

   ==================== ==================================== ===================== =========================================
   symbol               description                          unit                  variable name
   ==================== ==================================== ===================== =========================================
   :math:`q_{x}`        mass mixing ratio of air component x :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio {:}`
                        with regard to total air
   :math:`q_{dry\_air}` mass mixing ratio of dry air with    :math:`\frac{kg}{kg}` `dry_air_mass_mixing_ratio {:}`
                        regard to total air
   :math:`\bar{q}_{x}`  mass mixing ratio of air component x :math:`\frac{kg}{kg}` `<species>_mass_mixing_ratio_dry_air {:}`
                        with regard to dry air
   ==================== ==================================== ===================== =========================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      \bar{q}_{x} = \frac{q_{x}}{q_{dry\_air}}


   .. _derivation_dry_air_mass_mixing_ratio_from_H2O_mass_mixing_ratio:

#. dry air mass mixing ratio from H2O mass mixing ratio

   ==================== ============================ ===================== ===============================
   symbol               description                  unit                  variable name
   ==================== ============================ ===================== ===============================
   :math:`q_{H_{2}O}`   mass mixing ratio of H2O     :math:`\frac{kg}{kg}` `H2O_mass_mixing_ratio {:}`
                        with regard to total air
   :math:`q_{dry\_air}` mass mixing ratio of dry air :math:`\frac{kg}{kg}` `dry_air_mass_mixing_ratio {:}`
                        with regard to total air
   ==================== ============================ ===================== ===============================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{dry\_air} = 1 - q_{H_{2}O}


   .. _derivation_dry_air_mass_mixing_ratio_from_H2O_mass_mixing_ratio_dry_air:

#. dry air mass mixing ratio from H2O mass mixing ratio

   ======================== ============================ ===================== ===================================
   symbol                   description                  unit                  variable name
   ======================== ============================ ===================== ===================================
   :math:`\bar{q}_{H_{2}O}` mass mixing ratio of H2O     :math:`\frac{kg}{kg}` `H2O_mass_mixing_ratio_dry_air {:}`
                            with regard to dry air
   :math:`q_{dry\_air}`     mass mixing ratio of dry air :math:`\frac{kg}{kg}` `dry_air_mass_mixing_ratio {:}`
                            with regard to total air
   ======================== ============================ ===================== ===================================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{dry\_air} = \frac{1}{1 + \bar{q}_{H_{2}O}}


  .. _derivation_H2O_mass_mixing_ratio_from_dry_air_mass_mixing_ratio:

#. H2O mass mixing ratio from dry air mass mixing ratio

   ==================== ============================ ===================== ===============================
   symbol               description                  unit                  variable name
   ==================== ============================ ===================== ===============================
   :math:`q_{H_{2}O}`   mass mixing ratio of H2O     :math:`\frac{kg}{kg}` `H2O_mass_mixing_ratio {:}`
                        with regard to total air
   :math:`q_{dry\_air}` mass mixing ratio of dry air :math:`\frac{kg}{kg}` `dry_air_mass_mixing_ratio {:}`
                        with regard to total air
   ==================== ============================ ===================== ===============================

   The pattern `:` for the dimensions can represent `{vertical}`, `{latitude,longitude}`, `{latitude,longitude,vertical}`,
   `{time}`, `{time,vertical}`, `{time,latitude,longitude}`, `{time,latitude,longitude,vertical}`, or no dimensions at all.

   .. math::

      q_{H_{2}O} = 1 - q_{dry\_air}
